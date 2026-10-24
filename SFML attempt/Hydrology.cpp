#include "Hydrology.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <deque>
#include <sstream>

namespace {

	// Q in m3/s from a depth in mm per year over an area in km2: 1 mm over 1 km2 is 1000 m3
	constexpr double MM_KM2_TO_M3S = 1000.0 / (365.0 * 24.0 * 3600.0);
	constexpr double PRECIP_UNIT_MM = 10.0;  // Precipitation 300 is 3000 mm per year
	constexpr double HEIGHT_UNIT_M = 10000.0; // Height 1 is 10 km, the lapse rate convention of planetCalcTemp

	// A depression in the merge tree. A leaf is a local minimum. An inner node is what several
	// depressions become when the water rises over the pass between them
	struct Node {
		std::vector<int> own;       // Cells that joined this node directly (an inner node's own include its pass)
		std::vector<int> children;
		int parent = -1;
		double runoff = 0.0;        // Runoff of all member cells, m3/s
		double evap = 0.0;          // What the surface evaporates with every member cell under water, m3/s
		int count = 0;              // Member cells, children included
		float minHeight = std::numeric_limits<float>::max();
		float spillLevel = 0.f;     // The pass: height where this node meets a neighbour or the ocean
		int pass = -1;              // The cell at the pass
		int target = -1;            // The cell the water goes to over the pass
		int sink = 0;               // 0 nowhere (planet without ocean), 1 the ocean, 2 a sibling depression
		bool viable = false;
		double evapEff = 0.0;       // evap if viable, otherwise 0 so the water just passes
	};

	struct Neighbour { int cell; int root; };

} // namespace

HydrologyStats planetCalcHydrology(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config)
{
	HydrologyStats stats;
	std::vector<Cell>& cells = planet.cells;
	const int n = static_cast<int>(cells.size());
	globals.waterBodies.clear();
	if (n == 0) { return stats; }

	const double radiusKm = globals.planetaryParams.planetRadiusKm;

	// Per cell water balance. Land only, the ocean has no runoff
	std::vector<double> runoffQ(n, 0.0); // m3/s
	std::vector<double> evapQ(n, 0.0);   // m3/s if the cell is under a lake
	std::vector<double> areaKm2(n, 0.0);
	for (int i = 0; i < n; i++)
	{
		Cell& cell = cells[i];
		areaKm2[i] = static_cast<double>(planet.cellArea[i]) * radiusKm * radiusKm;
		cell.pet = config.pet_scale * std::max(0.f, cell.temp + 5.f);
		cell.aet = 0.f;
		cell.runoff = 0.f;
		cell.waterBody = -1;
		if (cell.oceanBool) { continue; }

		// Fu's Budyko curve: AET / P = 1 + phi - (1 + phi^w)^(1/w) with phi = PET / P
		const double precip = cell.percepitation;
		if (precip > 0.0)
		{
			const double phi = cell.pet / precip;
			const double w = config.budyko_w;
			double aet = precip * (1.0 + phi - std::pow(1.0 + std::pow(phi, w), 1.0 / w));
			aet = std::clamp(aet, 0.0, std::min(precip, static_cast<double>(cell.pet)));
			cell.aet = static_cast<float>(aet);
			cell.runoff = static_cast<float>(precip - aet);
		}
		runoffQ[i] = cell.runoff * PRECIP_UNIT_MM * areaKm2[i] * MM_KM2_TO_M3S;
		evapQ[i] = cell.pet * config.open_water_factor * PRECIP_UNIT_MM * areaKm2[i] * MM_KM2_TO_M3S;
		stats.landRunoff += runoffQ[i];
	}

	// Ocean first, then the land from the lowest cell up. The rank breaks ties between equal heights
	std::vector<int> order(n);
	std::iota(order.begin(), order.end(), 0);
	std::sort(order.begin(), order.end(), [&](int a, int b) {
		if (cells[a].oceanBool != cells[b].oceanBool) { return cells[a].oceanBool; }
		if (cells[a].height != cells[b].height) { return cells[a].height < cells[b].height; }
		return a < b;
		});
	std::vector<int> rank(n);
	for (int k = 0; k < n; k++) { rank[order[k]] = k; }
	int numOcean = 0;
	while (numOcean < n && cells[order[numOcean]].oceanBool) { numOcean++; }

	std::vector<int> dsu(n);
	std::iota(dsu.begin(), dsu.end(), 0);
	auto find = [&](int x) {
		while (dsu[x] != x) { dsu[x] = dsu[dsu[x]]; x = dsu[x]; }
		return x;
		};
	std::vector<int> compNode(n, -1); // At the root of a land component, its node
	std::vector<char> processed(n, 0);
	const int oceanRoot = numOcean > 0 ? order[0] : -1;
	for (int k = 0; k < numOcean; k++) { dsu[order[k]] = oceanRoot; processed[order[k]] = 1; }

	std::vector<Node> nodes;
	std::vector<int> naturalDown(n, -1); // Lowest neighbour that was there first
	std::vector<int> nodeOfPit(n, -1);
	const float minDepthM = config.min_lake_depth_m;
	const int minCells = static_cast<int>(config.min_lake_cells);

	auto addCell = [&](Node& node, int c) {
		node.own.push_back(c);
		node.runoff += runoffQ[c];
		node.evap += evapQ[c];
		node.count++;
		node.minHeight = std::min(node.minHeight, cells[c].height);
		};
	auto finalize = [&](Node& node, float level, int pass, int target, int sink) {
		node.spillLevel = level;
		node.pass = pass;
		node.target = target;
		node.sink = sink;
		const double depthM = (level - node.minHeight) * HEIGHT_UNIT_M;
		node.viable = node.count >= minCells && depthM >= minDepthM;
		node.evapEff = node.viable ? node.evap : 0.0;
		};

	// 1. The merge tree
	std::vector<Neighbour> around;
	std::vector<int> roots, landRoots;
	for (int k = numOcean; k < n; k++)
	{
		const int c = order[k];
		around.clear();
		roots.clear();
		int lowest = -1;
		for (int nb : cells[c].neighbors)
		{
			if (!processed[nb]) { continue; }
			const int r = find(nb);
			around.push_back({ nb, r });
			if (lowest < 0 || rank[nb] < rank[lowest]) { lowest = nb; }
			if (std::find(roots.begin(), roots.end(), r) == roots.end()) { roots.push_back(r); }
		}
		naturalDown[c] = lowest;
		processed[c] = 1;

		if (roots.empty())
		{
			// A local minimum
			dsu[c] = c;
			compNode[c] = static_cast<int>(nodes.size());
			nodeOfPit[c] = compNode[c];
			nodes.emplace_back();
			addCell(nodes.back(), c);
			continue;
		}
		if (roots.size() == 1)
		{
			dsu[c] = roots[0];
			if (roots[0] != oceanRoot) { addCell(nodes[compNode[roots[0]]], c); }
			continue;
		}

		// The cell joins several basins: it is the pass between them
		bool hasOcean = false;
		landRoots.clear();
		for (int r : roots)
		{
			if (r == oceanRoot) { hasOcean = true; }
			else { landRoots.push_back(r); }
		}
		// Lowest neighbour of the pass outside a basin: where the water goes when it spills
		auto targetOutside = [&](int root) {
			int best = -1;
			for (const Neighbour& nb : around)
			{
				if (nb.root != root && (best < 0 || rank[nb.cell] < rank[best])) { best = nb.cell; }
			}
			return best;
			};
		const float level = cells[c].height;

		if (hasOcean)
		{
			// Every basin here spills into the ocean, which swallows them
			int oceanTarget = -1;
			for (const Neighbour& nb : around)
			{
				if (nb.root == oceanRoot && (oceanTarget < 0 || rank[nb.cell] < rank[oceanTarget])) { oceanTarget = nb.cell; }
			}
			for (int r : landRoots)
			{
				finalize(nodes[compNode[r]], level, c, oceanTarget, 1);
				dsu[r] = oceanRoot;
			}
			dsu[c] = oceanRoot;
		}
		else
		{
			const int parent = static_cast<int>(nodes.size());
			nodes.emplace_back();
			for (int r : landRoots)
			{
				const int child = compNode[r];
				finalize(nodes[child], level, c, targetOutside(r), 2);
				Node& p = nodes[parent];
				const Node& ch = nodes[child];
				p.children.push_back(child);
				p.runoff += ch.runoff;
				p.evap += ch.evap;
				p.count += ch.count;
				p.minHeight = std::min(p.minHeight, ch.minHeight);
				nodes[child].parent = parent;
				dsu[r] = c;
			}
			// Siblings that cannot hold the water all send it to the largest one, so two small ones never
			// point at each other
			int recipient = -1;
			for (int r : landRoots)
			{
				const Node& cand = nodes[compNode[r]];
				const Node* best = recipient < 0 ? nullptr : &nodes[compNode[recipient]];
				// Most evaporation wins, and with none the basin with the lowest pit: a lake fills the lowest cells first
				if (!best || cand.evapEff > best->evapEff || (cand.evapEff == best->evapEff && cand.minHeight < best->minHeight)) { recipient = r; }
			}
			for (int r : landRoots)
			{
				if (r == recipient) { continue; }
				int best = -1;
				for (const Neighbour& nb : around)
				{
					if (nb.root == recipient && (best < 0 || rank[nb.cell] < rank[best])) { best = nb.cell; }
				}
				if (best >= 0) { nodes[compNode[r]].target = best; }
			}
			dsu[c] = c;
			compNode[c] = parent;
			addCell(nodes[parent], c);
		}
	}
	// What is left has nowhere to go: a planet without an ocean
	const float highest = numOcean < n ? cells[order[n - 1]].height : 0.f;
	for (Node& node : nodes)
	{
		if (node.parent < 0 && node.sink == 0 && node.spillLevel == 0.f && node.pass < 0)
		{
			finalize(node, highest, -1, -1, 0);
		}
	}

	stats.nodes = static_cast<int>(nodes.size());
	for (const Node& node : nodes)
	{
		if (node.children.empty()) { stats.leaves++; }
		if (node.viable) { stats.viableNodes++; }
	}

	// Where every cell's water ends up: the local minimum its lowest-neighbour chain reaches, and the top of that
	// minimum's tree. Cells above a basin's pass that drain into it belong to its catchment too
	std::vector<int> rootOf(nodes.size());
	for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; i--)
	{
		rootOf[i] = nodes[i].parent < 0 ? i : rootOf[nodes[i].parent];
	}
	std::vector<int> regionRoot(n, -1); // -1 for the ocean
	std::vector<double> catchment(nodes.size(), 0.0);
	for (int k = numOcean; k < n; k++)
	{
		const int c = order[k];
		const int down = naturalDown[c];
		if (down < 0) { regionRoot[c] = rootOf[nodeOfPit[c]]; }
		else if (!cells[down].oceanBool) { regionRoot[c] = regionRoot[down]; }
		if (regionRoot[c] >= 0) { catchment[regionRoot[c]] += runoffQ[c]; }
	}

	// 2. Lakes, from the top of the tree down
	std::vector<int> downstream = naturalDown;
	std::vector<double> inject(n, 0.0);
	std::vector<int> stamp(n, -1);
	int stampId = 0;

	auto gather = [&](int root, std::vector<int>& out) {
		std::vector<int> stack{ root };
		while (!stack.empty())
		{
			const int ni = stack.back();
			stack.pop_back();
			out.insert(out.end(), nodes[ni].own.begin(), nodes[ni].own.end());
			for (int ch : nodes[ni].children) { stack.push_back(ch); }
		}
		};

	// Links the pass to the cell on the far side unless that cell already drains back to the pass
	auto linkPass = [&](int pass, int target) {
		if (pass < 0 || target < 0) { return; }
		int x = target;
		for (int steps = 0; x >= 0 && steps <= n; steps++)
		{
			if (x == pass) { return; }
			x = downstream[x];
		}
		downstream[pass] = target;
		};

	// A depression too small to be a lake: the water is carried over the flooded part to the pass
	auto passThrough = [&](int ni) {
		const Node& node = nodes[ni];
		stats.passThroughRegions++;
		if (node.sink == 0 || node.pass < 0) { return; }
		std::vector<int> members;
		gather(ni, members);
		stampId++;
		for (int m : members) { stamp[m] = stampId; }
		// If the water that leaves over the pass comes back into this depression there is no way out
		{
			int x = node.target;
			for (int steps = 0; x >= 0 && steps <= n; steps++)
			{
				if (stamp[x] == stampId || x == node.pass) { return; }
				x = downstream[x];
			}
		}
		std::vector<int> queue;
		for (int nb : cells[node.pass].neighbors)
		{
			if (stamp[nb] == stampId && cells[nb].waterBody < 0)
			{
				stamp[nb] = -2;
				downstream[nb] = node.pass;
				queue.push_back(nb);
			}
		}
		for (std::size_t head = 0; head < queue.size(); head++)
		{
			const int x = queue[head];
			for (int nb : cells[x].neighbors)
			{
				if (stamp[nb] == stampId && cells[nb].waterBody < 0 && cells[nb].height < node.spillLevel)
				{
					stamp[nb] = -2;
					downstream[nb] = x;
					queue.push_back(nb);
				}
			}
		}
		linkPass(node.pass, node.target);
		};

	// Returns what leaves over the pass, which only counts for the top of a tree
	auto makeLake = [&](int ni, double inflow) -> double {
		const Node& node = nodes[ni];
		if (!node.viable) { passThrough(ni); return inflow; }
		if (inflow <= 0.0) { return 0.0; }

		std::vector<int> members;
		gather(ni, members);
		std::sort(members.begin(), members.end(), [&](int a, int b) { return rank[a] < rank[b]; });

		WaterBody body;
		const bool full = inflow >= node.evap;
		if (full)
		{
			body.cells = members;
			body.level = node.spillLevel;
			body.evaporation = static_cast<float>(node.evap);
			// Inside a depression the surplus was already handed to the neighbours, what is left is rounding
			body.outflow = node.parent < 0 ? static_cast<float>(inflow - node.evap) : 0.f;
		}
		else
		{
			double cumulative = 0.0;
			std::size_t k = 0;
			while (k < members.size() && cumulative < inflow) { cumulative += evapQ[members[k]]; k++; }
			body.cells.assign(members.begin(), members.begin() + k);
			body.level = cells[members[k - 1]].height;
			body.evaporation = static_cast<float>(inflow);
		}
		body.full = full;
		body.inflow = static_cast<float>(inflow);
		body.minHeight = cells[members.front()].height;
		body.depthM = (body.level - body.minHeight) * static_cast<float>(HEIGHT_UNIT_M);
		for (int m : body.cells) { body.areaKm2 += static_cast<float>(areaKm2[m]); }
		stats.lakeEvaporation += body.evaporation;

		if (full)
		{
			if (node.sink == 0) { stats.lost += body.outflow; body.outflow = 0.f; }
			else if (body.outflow > 0.f)
			{
				body.outlet = node.pass;
				body.outletTarget = node.target;
				inject[node.pass] += body.outflow;
				linkPass(node.pass, node.target);
			}
		}
		else { stats.closedLakes++; }

		const int id = static_cast<int>(globals.waterBodies.size());
		for (int m : body.cells) { cells[m].waterBody = id; }
		globals.waterBodies.push_back(std::move(body));
		stats.lakes++;
		return full && node.parent < 0 ? static_cast<double>(inflow - node.evap) : 0.0;
		};

	// A tree whose pass opens into another tree's catchment hands its outflow on, so the highest pass goes first
	std::vector<int> treeRoots;
	for (int i = 0; i < static_cast<int>(nodes.size()); i++)
	{
		if (nodes[i].parent < 0) { treeRoots.push_back(i); }
	}
	std::sort(treeRoots.begin(), treeRoots.end(), [&](int a, int b) {
		const int ra = nodes[a].pass >= 0 ? rank[nodes[a].pass] : -1;
		const int rb = nodes[b].pass >= 0 ? rank[nodes[b].pass] : -1;
		return ra > rb;
		});
	std::vector<double> incoming(nodes.size(), 0.0);
	std::vector<char> done(nodes.size(), 0);

	std::vector<std::pair<int, double>> stack;
	for (int treeRoot : treeRoots)
	{
	stack.push_back({ treeRoot, catchment[treeRoot] + incoming[treeRoot] });
	done[treeRoot] = 1;
	double leaving = 0.0;
	while (!stack.empty())
	{
		const auto [ni, inflow] = stack.back();
		stack.pop_back();
		const Node& node = nodes[ni];

		double childEvap = 0.0;
		for (int ch : node.children) { childEvap += nodes[ch].evapEff; }
		if (node.children.empty() || inflow >= childEvap)
		{
			// One lake, over the whole depression
			const double out = makeLake(ni, inflow);
			if (ni == treeRoot) { leaving = out; }
			continue;
		}

		// The depressions below the pass do not merge. The ones that receive more than they evaporate
		// spill the surplus to the others, in proportion to what they still lack
		double childRunoff = 0.0, pool = 0.0, lacking = 0.0;
		for (int ch : node.children)
		{
			const Node& c = nodes[ch];
			childRunoff += c.runoff;
			if (c.runoff > c.evapEff)
			{
				const double surplus = c.runoff - c.evapEff;
				pool += surplus;
				if (c.viable && c.sink != 0)
				{
					inject[c.pass] += surplus;
					linkPass(c.pass, c.target);
				}
			}
			else { lacking += c.evapEff - c.runoff; }
		}
		pool += std::max(0.0, inflow - childRunoff);
		for (int ch : node.children)
		{
			const Node& c = nodes[ch];
			double eff = std::min(c.runoff, c.evapEff);
			if (c.runoff < c.evapEff && lacking > 0.0) { eff += pool * (c.evapEff - c.runoff) / lacking; }
			stack.push_back({ ch, std::min(eff, c.evapEff) });
		}
	}
	// Over the pass into the catchment of a tree that has a lower pass, which then has to carry it
	if (leaving > 0.0 && nodes[treeRoot].sink == 1 && nodes[treeRoot].target >= 0)
	{
		const int target = nodes[treeRoot].target;
		const int region = cells[target].oceanBool ? -1 : regionRoot[target];
		if (region >= 0 && region != treeRoot)
		{
			if (done[region]) { stats.lost += leaving; }
			else { incoming[region] += leaving; }
		}
	}
	}

	// A pit that ended up neither under a lake nor crossed (its depression was merged into a lake that only
	// fills its lowest cells) sends its water over its own pass, to the sibling basin that has the lake
	for (int i = 0; i < n; i++)
	{
		if (cells[i].oceanBool || naturalDown[i] >= 0 || cells[i].waterBody >= 0 || nodeOfPit[i] < 0 || downstream[i] >= 0) { continue; }
		passThrough(nodeOfPit[i]);
	}

	// 3. Discharge. Cells with nothing upstream first, each one passes its sum to the cell it flows to.
	// The links through the flooded part of a crossed depression do not follow the rank, hence the sort
	std::vector<double> discharge(n, 0.0);
	std::vector<int> upstreamLeft(n, 0);
	for (int i = 0; i < n; i++)
	{
		discharge[i] = runoffQ[i] + inject[i];
		if (downstream[i] >= 0) { upstreamLeft[downstream[i]]++; }
	}
	std::vector<int> ready;
	for (int i = 0; i < n; i++)
	{
		if (upstreamLeft[i] == 0) { ready.push_back(i); }
	}
	std::vector<int> stuck; // Pits that still have nowhere to send the water
	int sorted = 0;
	while (!ready.empty())
	{
		const int c = ready.back();
		ready.pop_back();
		sorted++;
		const int to = downstream[c];
		if (to < 0)
		{
			if (!cells[c].oceanBool && cells[c].waterBody < 0) { stuck.push_back(c); }
			continue;
		}
		discharge[to] += discharge[c];
		if (cells[to].oceanBool && !cells[c].oceanBool) { stats.toOcean += discharge[c]; }
		if (--upstreamLeft[to] == 0) { ready.push_back(to); }
	}
	stats.cyclicCells = n - sorted;

	// Whatever is left in a pit becomes a one-cell closed lake, so no river ends on dry land
	for (int c : stuck)
	{
		if (discharge[c] <= 0.0) { continue; }
		WaterBody body;
		body.cells = { c };
		body.level = body.minHeight = cells[c].height;
		body.areaKm2 = static_cast<float>(areaKm2[c]);
		body.inflow = body.evaporation = static_cast<float>(discharge[c]);
		cells[c].waterBody = static_cast<int>(globals.waterBodies.size());
		globals.waterBodies.push_back(std::move(body));
		stats.lakes++;
		stats.closedLakes++;
		stats.deadEnd += discharge[c];
		stats.lakeEvaporation += discharge[c];
	}
	for (int i = 0; i < n; i++)
	{
		cells[i].discharge = static_cast<float>(discharge[i]);
		cells[i].downstream = cells[i].oceanBool ? -1 : downstream[i];
	}

	std::ostringstream out;
	out << stats.nodes << " depressions (" << stats.leaves << " minima, " << stats.viableNodes << " deep enough), "
		<< stats.lakes << " lakes (" << stats.closedLakes << " closed), " << stats.passThroughRegions << " crossed by the river. "
		<< "Runoff " << stats.landRunoff << " m3/s = lake evaporation " << stats.lakeEvaporation << " + to ocean " << stats.toOcean
		<< " + lost " << stats.lost << ", dead ends " << stats.deadEnd << ", cyclic cells " << stats.cyclicCells;
	stats.report = out.str();
	return stats;
}

RiverStats planetCalcRivers(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config)
{
	RiverStats stats;
	std::vector<Cell>& cells = planet.cells;
	const int n = static_cast<int>(cells.size());
	const double radiusKm = globals.planetaryParams.planetRadiusKm;

	globals.rivers.clear();
	globals.riverCells.clear();
	globals.lakeCells.clear();
	for (int i = 0; i < n; i++)
	{
		Cell& cell = cells[i];
		cell.riverBool = false;
		cell.riverId = -1;
		cell.riverWidth = cell.riverDepth = cell.riverSpeed = 0.f;
		cell.lakeBool = cell.waterBody >= 0;
		cell.lakeId = cell.waterBody;
		if (cell.lakeBool) { globals.lakeCells.push_back(i); stats.lakeCells++; }
		if (!cell.oceanBool && !cell.lakeBool && cell.discharge >= config.river_min_discharge)
		{
			cell.riverBool = true;
			globals.riverCells.push_back(i);
			stats.riverCells++;
		}
	}
	for (WaterBody& body : globals.waterBodies) { body.inflowRivers.clear(); body.outflowRiver = -1; }

	// Distance between two cell centres in km
	auto distanceKm = [&](int a, int b) {
		const double d = std::clamp(static_cast<double>(glm::dot(planet.points[a], planet.points[b])), -1.0, 1.0);
		return std::acos(d) * radiusKm;
		};

	// 1. Rivers: from each mouth upstream along the larger branch, the others become tributaries
	std::vector<std::vector<int>> upstream(n);
	std::vector<int> mouths;
	for (int c : globals.riverCells)
	{
		const int down = cells[c].downstream;
		if (down >= 0 && cells[down].riverBool) { upstream[down].push_back(c); }
		else { mouths.push_back(c); }
	}
	std::sort(mouths.begin(), mouths.end(), [&](int a, int b) { return cells[a].discharge > cells[b].discharge; });

	struct Work { int top; int parent; int confluence; };
	std::deque<Work> work;
	for (int m : mouths) { work.push_back({ m, -1, -1 }); }
	while (!work.empty())
	{
		const Work job = work.front();
		work.pop_front();
		const int id = static_cast<int>(globals.rivers.size());
		globals.rivers.emplace_back(id);

		std::vector<int> walk;
		for (int cur = job.top;;)
		{
			walk.push_back(cur);
			cells[cur].riverId = id;
			int best = -1;
			for (int up : upstream[cur])
			{
				if (best < 0 || cells[up].discharge > cells[best].discharge) { best = up; }
			}
			for (int up : upstream[cur])
			{
				if (up != best) { work.push_back({ up, id, cur }); }
			}
			if (best < 0) { break; }
			cur = best;
		}

		River& river = globals.rivers[id];
		river.pathCells.assign(walk.rbegin(), walk.rend());
		river.mouthDischarge = cells[job.top].discharge;
		river.parentRiver = job.parent;
		if (job.parent >= 0)
		{
			river.pathCells.push_back(job.confluence);
			river.mouthType = RiverMouth::River;
			globals.rivers[job.parent].tributaries.push_back(id);
		}
		else
		{
			stats.mainRivers++;
			const int down = cells[job.top].downstream;
			if (down >= 0 && cells[down].oceanBool) { river.mouthType = RiverMouth::Ocean; }
			else if (down >= 0 && cells[down].waterBody >= 0)
			{
				river.mouthType = RiverMouth::Lake;
				river.mouthLake = cells[down].waterBody;
				globals.waterBodies[river.mouthLake].inflowRivers.push_back(id);
			}
		}
		for (std::size_t k = 1; k < river.pathCells.size(); k++)
		{
			river.lengthKm += static_cast<float>(distanceKm(static_cast<int>(river.pathCells[k - 1]), static_cast<int>(river.pathCells[k])));
		}
		if (job.parent < 0 && cells[job.top].downstream >= 0) { river.lengthKm += static_cast<float>(distanceKm(job.top, cells[job.top].downstream)); }
		stats.longestKm = std::max(stats.longestKm, river.lengthKm);
	}
	stats.rivers = static_cast<int>(globals.rivers.size());

	for (WaterBody& body : globals.waterBodies)
	{
		if (body.outlet >= 0 && cells[body.outlet].riverBool) { body.outflowRiver = cells[body.outlet].riverId; }
	}

	// 2. Width, depth and speed. The slope is the drop of the filled water surface (a lake is level, a
	// depression that was crossed is as high as its pass) over the distance to the next cell
	constexpr double MANNING_N = 0.035;
	constexpr double MIN_SLOPE = 1e-5;
	std::vector<float> filled(n, std::numeric_limits<float>::quiet_NaN());
	auto filledHeight = [&](int start) {
		std::vector<int> chain;
		int x = start;
		float base = 0.f;
		while (true)
		{
			if (!std::isnan(filled[x])) { base = filled[x]; break; }
			if (cells[x].oceanBool) { filled[x] = cells[x].height; base = filled[x]; break; }
			if (cells[x].waterBody >= 0) { filled[x] = globals.waterBodies[cells[x].waterBody].level; base = filled[x]; break; }
			chain.push_back(x);
			if (cells[x].downstream < 0) { base = -std::numeric_limits<float>::max(); break; }
			x = cells[x].downstream;
		}
		for (auto it = chain.rbegin(); it != chain.rend(); ++it)
		{
			base = std::max(base, cells[*it].height);
			filled[*it] = base;
		}
		return filled[start];
		};
	for (int c : globals.riverCells)
	{
		Cell& cell = cells[c];
		const int down = cell.downstream;
		double slope = MIN_SLOPE;
		if (down >= 0)
		{
			const double drop = (static_cast<double>(filledHeight(c)) - filledHeight(down)) * HEIGHT_UNIT_M;
			slope = std::max(MIN_SLOPE, drop / (distanceKm(c, down) * 1000.0));
		}
		const double q = cell.discharge;
		const double width = 4.0 * std::sqrt(q);
		const double depth = std::pow(q * MANNING_N / (width * std::sqrt(slope)), 0.6);
		cell.riverWidth = static_cast<float>(width);
		cell.riverDepth = static_cast<float>(depth);
		cell.riverSpeed = static_cast<float>(q / (width * depth));
		stats.maxDischarge = std::max(stats.maxDischarge, cell.discharge);
		stats.maxWidth = std::max(stats.maxWidth, cell.riverWidth);
		stats.maxDepth = std::max(stats.maxDepth, cell.riverDepth);
		stats.maxSpeed = std::max(stats.maxSpeed, cell.riverSpeed);
	}

	std::ostringstream out;
	out << stats.rivers << " rivers (" << stats.mainRivers << " main), " << stats.riverCells << " river cells, " << stats.lakeCells
		<< " lake cells, longest " << stats.longestKm << " km, max " << stats.maxDischarge << " m3/s, " << stats.maxWidth << " m wide, "
		<< stats.maxDepth << " m deep, " << stats.maxSpeed << " m/s";
	stats.report = out.str();
	return stats;
}