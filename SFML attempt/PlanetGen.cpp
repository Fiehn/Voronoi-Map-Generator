#include "PlanetGen.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <glm/gtc/constants.hpp>
#include "SphereMath.hpp"
#include "util.hpp"

//////////
// Steps that only walk the cell graph, moved unchanged from Map.hpp
//////////
void rise(std::vector<Cell>& map)
{ /* Calculate the rise by finding the tallest and shortest neighbor*/
    // Needs to be optimized or rethought
    float max_height = std::numeric_limits<float>::min();
    float min_height = std::numeric_limits<float>::max();

#pragma omp parallel for num_threads(16) schedule(static)
    for (size_t i = 0; i < map.size(); i++)
    {
        max_height = std::numeric_limits<float>::min();
        min_height = std::numeric_limits<float>::max();

        // Cache neighbor heights
        const std::vector<int>& neighbors = map[i].neighbors;
        std::vector<float> neighbor_heights(neighbors.size());
        for (size_t j = 0; j < neighbors.size(); j++) {
            neighbor_heights[j] = map[neighbors[j]].height;
        }

        // find min and max height
        for (int j = 0; j < neighbors.size(); j++)
        {
            float neighbor_height = neighbor_heights[j];
            if (neighbor_height < min_height) min_height = neighbor_height;
            if (neighbor_height > max_height) max_height = neighbor_height;
        }

#pragma omp critical
        map[i].rise = max_height - min_height;
    }
}

void smooth_height(std::vector<Cell>& map, float rise_threshold, int repeats, int method)
{ // method 1 = Random, method 2 = Front
    {
        std::vector<unsigned int> active;
        active.reserve(map.size() * 5);

        for (int _ = 0; _ < repeats; _++)
        {
            for (size_t i = 0; i < map.size(); i++)
            {
                if (map[i].rise > rise_threshold)
                {
                    active.push_back(i);
                    active.insert(std::end(active), std::begin(map[i].neighbors), std::end(map[i].neighbors));
                }
            }
            while (true)
            {
                if (active.empty()) { break; }

                size_t index = 0;
                if (method == 1) { index = pop_random_i(active); }
                else if (method == 2) { index = pop_front_i(active); }

                float height_sum = 0.f;
                int count_values = 0;
                for (int j = 0; j < map[index].neighbors.size(); j++)
                {
                    height_sum = height_sum + map[map[index].neighbors[j]].height;
                    count_values++;
                }

                if (count_values > 0) {
                    map[index].height = height_sum / static_cast<float>(count_values);
                }
            }
            // calculate the new rise for next smoothing
            rise(map);
        }
    }
}

void assignPlateTypes(std::vector<Continent>& continents, const MapConfig& config)
{
	// Determine amount of oceanic and continental plates
	int numOceanicPlates = static_cast<int>(continents.size() * config.oceanic_plate_ratio);
    int numMixedPlates = static_cast<int>(continents.size() * 0.15f);
	int numContinentalPlates = static_cast<int>(continents.size()) - numOceanicPlates - numMixedPlates;

	// Randomly assign plate types
	std::vector<int> plateIndices(continents.size());
	std::iota(plateIndices.begin(), plateIndices.end(), 0);
	std::shuffle(plateIndices.begin(), plateIndices.end(), std::default_random_engine(static_cast<unsigned int>(time(0))));

    int assignedCount = 0;

    for (int i = 0; i < numOceanicPlates; i++)
    {
		continents[plateIndices[assignedCount]].plateType = PlateType::Oceanic;
        continents[plateIndices[assignedCount]].crustThickness = RandomBetween(
            config.oceanic_crust_thickness * 0.8f,
            config.oceanic_crust_thickness * 1.2f
        );
		assignedCount++;
    }
    for (int i = 0; i < numMixedPlates; i++)
    {
		continents[plateIndices[assignedCount]].plateType = PlateType::Mixed;
        continents[plateIndices[assignedCount]].crustThickness = RandomBetween(
            (config.oceanic_crust_thickness + config.continental_crust_thickness) / 2.0f * 0.8f,
            (config.oceanic_crust_thickness + config.continental_crust_thickness) / 2.0f * 1.2f
		);
		assignedCount++;
    }
    for (int i = 0; i < numContinentalPlates; i++)
    {
        continents[plateIndices[assignedCount]].plateType = PlateType::Continental;
        continents[plateIndices[assignedCount]].crustThickness = RandomBetween(
            config.continental_crust_thickness * 0.8f,
            config.continental_crust_thickness * 1.2f
        );
        assignedCount++;
    }
}

void setBaseElevations(std::vector<Cell>& map, GlobalWorldObjects& globals)
{
	// Set for each cell based on its continent
    for (std::size_t i = 0; i < map.size(); i++)
    {
		int continentId = map[i].continent;
        if (continentId >= 0 && continentId < globals.continents.size())
        {
            Continent& continent = globals.continents[continentId];

            float baseHeight = 0.0f;

            switch (continent.plateType)
            {
            case PlateType::Oceanic:
            {
                // Ocean floor: normal distribution around -0.4 (deep ocean)
                baseHeight = normalDistPDF(-0.4f, 0.15f);
                baseHeight += continent.getIsostaticHeight();
                // Don't clamp here - allow deep trenches!
                break;
            }

            case PlateType::Continental:
            {
                // Continental: normal distribution around 0.5
                baseHeight = normalDistPDF(0.5f, 0.2f);
                baseHeight += continent.getIsostaticHeight();
                // Don't clamp here - allow high plateaus!
                break;
            }

            case PlateType::Mixed:
            {
                // Mixed plates: bimodal
                if (RandomBetween(0.0f, 1.0f) < 0.6f) {
                    baseHeight = normalDistPDF(-0.15f, 0.1f);
                }
                else {
                    baseHeight = normalDistPDF(0.2f, 0.12f);
                }
                baseHeight += continent.getIsostaticHeight();
                break;
            }
            }
            float microVariation = normalDistPDF(0.0f, 0.02f);
            map[i].height = baseHeight + microVariation;
        }
    }
}

float calculateBoundaryHeightModifier(BoundaryType boundaryType,
    float dotProduct,
    float continent1Age,
    float continent2Age)
{
	float ageFactor = (continent1Age + continent2Age) / 2.0f;
    switch (boundaryType)
    {
    case BoundaryType::ContinentalCollision:
        return RandomBetween(0.3f, 0.6f) * ageFactor * std::abs(dotProduct);
    case BoundaryType::Subduction:
        return RandomBetween(0.2f, 0.4f) * ageFactor * std::abs(dotProduct);
    case BoundaryType::OceanicRift:
        return -RandomBetween(0.1f, 0.2f) * ageFactor;
    case BoundaryType::ContinentalRift:
        return -RandomBetween(0.05f, 0.15f) * ageFactor;
    case BoundaryType::Transform:
        return RandomBetween(-0.05f, 0.05f);
    default:
        return 0.0f;
    }
}

void simulateErosion(std::vector<Cell>& map, GlobalWorldObjects& globals, const MapConfig& config)
{
    if (!config.enable_erosion) {
        return;
	}
    for (int iteration = 0; iteration < config.erosion_iterations; iteration++)
    {
        std::vector<float> heightDeltas(map.size(), 0.0f);
        for (std::size_t i = 0; i < map.size(); i++) {
            float erosionRate = 0.0f;
			// Apply erosion based on height above sea level (relative to seaLevel)
            float heightAboveSeaLevel = map[i].height - globals.seaLevel;
            if (heightAboveSeaLevel > 0.1f) {
				erosionRate = heightAboveSeaLevel * config.erosion_strength;
            }
			// Apply erosion based on rise (steepness)
            erosionRate += map[i].rise * config.erosion_strength * 0.5f;
			// distribute erosion to neighbors
			float totalHeightDiff = 0.0f;
			std::vector<float> heightDiffs(map[i].neighbors.size());
            for (std::size_t j = 0; j < map[i].neighbors.size(); j++)
            {
				int neighborIdx = map[i].neighbors[j];
				float diff = map[i].height - map[neighborIdx].height;
                if (diff > 0) {
                    heightDiffs[j] = diff;
                    totalHeightDiff += diff;
                }
            }
			// remove from current cell and distribute
            heightDeltas[i] -= erosionRate;
            if (totalHeightDiff > 0.001f) {
				float sediment = erosionRate * 0.7f; // 30% of eroded material is lost
                for (std::size_t j = 0; j < map[i].neighbors.size(); j++)
                {
                    int neighborIdx = map[i].neighbors[j];
                    float proportion = heightDiffs[j] / totalHeightDiff;
                    heightDeltas[neighborIdx] += sediment * proportion;
                }
            }
        }
        // Apply height deltas (no floor - allow negative heights for deep trenches)
        for (std::size_t i = 0; i < map.size(); i++) {
            map[i].height += heightDeltas[i];
        }

		rise(map); // Recalculate rise after erosion
    }
}

void propagateTerrainFeatures(std::vector<Cell>& map,
    GlobalWorldObjects& globals,
    std::size_t startCellIndex,
    float heightIncrease,
    bool volcanicActivity,
    BoundaryType boundaryType,
    int propagation_depth)
{
    int continentId = map[startCellIndex].continent;
    if (continentId < 0 || continentId >= globals.continents.size()) {
        return;
    }
    // determined propagation depth based on boundary type and continent age
    int propagationDepthAd = 0;
    float propagationStrength = 0.0f;
    switch (boundaryType)
    {
    case BoundaryType::ContinentalCollision:
        propagationDepthAd = static_cast<int>(3 + 8 * globals.continents[continentId].getAge());
        propagationStrength = 0.5f;
        break;
    case BoundaryType::Subduction:
        propagationDepthAd = static_cast<int>(2 + 6 * globals.continents[continentId].getAge());
        propagationStrength = 0.3f;
        break;
    case BoundaryType::OceanicRift:
        propagationDepthAd = static_cast<int>(1 + 5 * globals.continents[continentId].getAge());
        propagationStrength = 0.2f;
        break;
    case BoundaryType::ContinentalRift:
        propagationDepthAd = static_cast<int>(1 + 5 * globals.continents[continentId].getAge());
        propagationStrength = 0.1f;
        break;
    case BoundaryType::Transform:
        propagationDepthAd = 1;
        propagationStrength = 0.05f;
        break;
    default:
        return;
    }
    int propagationDepth = propagation_depth * propagationDepthAd;

    // BFS propagation
    std::vector<std::size_t> visited(map.size(), false);
    visited[startCellIndex] = true;

    Queue<std::pair<std::size_t, int>> propagationQ;
    propagationQ.push(std::make_pair(startCellIndex, 0));
    while (!propagationQ.empty())
    {
        auto [currentCellIndex, depth] = propagationQ.pop_front();
        if (depth >= propagationDepth) {
            continue;
        }
        // calculate falloff with distance
        float distanceFactor = 1.0f - (static_cast<float>(depth) / static_cast<float>(propagationDepth));
        float propagationIncrease = heightIncrease * propagationStrength * distanceFactor;
        // add to neighbors on the same continent
        for (std::size_t neighborIdx : map[currentCellIndex].neighbors)
        {
            if (!visited[neighborIdx] && map[neighborIdx].continent == continentId)
            {
                visited[neighborIdx] = true;
                map[neighborIdx].height += propagationIncrease;
                if (volcanicActivity && RandomBetween(0.0f, 1.0f) < 0.3f * distanceFactor)
                {
                    map[neighborIdx].volcanicActivity = true;
                    map[neighborIdx].height = map[neighborIdx].height + RandomBetween(0.05f, 0.1f) * distanceFactor;
                }
                propagationQ.push(std::make_pair(neighborIdx, depth + 1));
            }
        }
    }
}

void cleanUpContinents(std::vector<Cell>& map, GlobalWorldObjects& globals)
{
    // Remove empty continents and reassign continent IDs
    std::vector<Continent> validContinents;
    validContinents.reserve(globals.continents.size());
    for (const auto& continent : globals.continents)
    {
        if (!continent.cells.empty())
        {
            validContinents.push_back(continent);
        }
    }
    // Reassign continent IDs in cells
    for (std::size_t i = 0; i < map.size(); i++)
    {
        int oldContinentId = map[i].continent;
        if (oldContinentId >= 0 && oldContinentId < globals.continents.size())
        {
            const Continent& oldContinent = globals.continents[oldContinentId];
            auto it = std::find_if(validContinents.begin(), validContinents.end(),
                [&oldContinent](const Continent& c) { return c.id == oldContinent.id; });
            if (it != validContinents.end())
            {
                map[i].continent = static_cast<int>(std::distance(validContinents.begin(), it));
            }
            else
            {
                map[i].continent = -1; // No valid continent
            }
        }
        else
        {
            map[i].continent = -1; // No valid continent
        }
    }
    globals.continents = std::move(validContinents);
}

void generateOceans(std::vector<Cell>& map, GlobalWorldObjects& globals)
{
    // Alogrithm:
	// 1. Initialize ocean cells on each Oceanic plate where the cell is below sea level
	// 2. Use BFS (starting on border cells) to propagate ocean cells to neighboring cells below sea level
	// 3. Mark cells as ocean and add to global ocean cell list

	std::vector<bool> visited(map.size(), false);
	Queue<std::size_t> oceanQ;
	// Step 1: Initialize ocean cells on Oceanic plates
    for (std::size_t i = 0; i < globals.continents.size(); i++)
    {
        if (globals.continents[i].plateType == PlateType::Oceanic)
        {
            for (std::size_t cellIdx : globals.continents[i].cells)
            {
                if (map[cellIdx].height <= globals.seaLevel && !visited[cellIdx])
                {
                    visited[cellIdx] = true;
                    map[cellIdx].oceanBool = true;
                    globals.oceanCells.push_back(cellIdx);
					oceanQ.push(cellIdx);
				}
            }
        }
    }
	// Step 2: BFS to propagate ocean cells
    while (!oceanQ.empty())
    {
		std::size_t currentIdx = oceanQ.pop_front();
        for (std::size_t neighborIdx : map[currentIdx].neighbors)
        {
			// Add neighbor if below sea level and not visited
            if (!visited[neighborIdx] && map[neighborIdx].height <= globals.seaLevel)
            {
                visited[neighborIdx] = true;
                map[neighborIdx].oceanBool = true;
                globals.oceanCells.push_back(neighborIdx);
                oceanQ.push(neighborIdx);
            }
            // Mark coast cells (neighbors above sea level)
            else if (!visited[neighborIdx] && map[neighborIdx].height > globals.seaLevel)
            {
                visited[neighborIdx] = true;
                map[neighborIdx].oceanBool = false;
                map[neighborIdx].coastBool = true;
                globals.coastCells.push_back(neighborIdx);
            }
        }
    }
}

void generateSnowline(std::vector<Cell>& map, GlobalWorldObjects& globals)
{
	// Generate snowline cells
    // If above snowline heigh
	// and temperature is below 5C
	// mark as snowline
    for (size_t i = 0; i < map.size(); i++)
    {
        if (map[i].height >= globals.globalSnowline && map[i].temp <= 5.0f)
        {
            map[i].snowBool = true;
            globals.snowCells.push_back(i);
        }
	}
}

void generateTreeline(std::vector<Cell>& map, GlobalWorldObjects& globals)
{
    // Generate treeline cells
	// Should be temperature and height based 
    for (size_t i = 0; i < map.size(); i++)
    {
        map[i].treeBool = true;
        globals.treeCells.push_back(i);
	}
}

void calcHeightValues(std::vector<Cell>& map, GlobalWorldObjects& globals, float deltaCoastLine) // sea level, coast, treeline and snow line
{
	// USE DELTA COASTLINE TO ADJUST coast lines later
    
    //map[i].height = clamp(map[i].height, 1.0, 0.0);

    // Generate oceans and coasts
	generateOceans(map, globals);

    // Snowline
	generateSnowline(map, globals);
        
	// Treeline
	generateTreeline(map, globals);
    
}

void closeOceanCell(std::vector<Cell>& map)
{   // Steps from every cell to the closest cell on the other side of the coast, both sides use distToOcean:
    // land cells get their distance to the ocean, ocean cells reuse the field for their distance to land.
    // Cells that can not reach the other side keep std::numeric_limits<int>::max()
    // Land cells next to the ocean are marked as coast.
    const int unreached = std::numeric_limits<int>::max();

    for (bool fromOcean : { true, false })
    {
        // Breadth first from every cell of one side at once. A cell is marked when it is queued, so it is queued once
        // and its first distance is already the shortest
        std::vector<int> steps(map.size(), unreached);
        Queue<int> queue;
        for (int i = 0; i < map.size(); i++)
        {
            if (map[i].oceanBool == fromOcean)
            {
                steps[i] = 0;
                queue.push(i);
            }
        }
        while (!queue.empty())
        {
            int idx = queue.pop_front();
            for (int neighbor : map[idx].neighbors)
            {
                if (steps[neighbor] == unreached)
                {
                    steps[neighbor] = steps[idx] + 1;
                    queue.push(neighbor);
                }
            }
        }

        // Only the other side is written, the sources are 0 by definition
        for (int i = 0; i < map.size(); i++)
        {
            if (map[i].oceanBool != fromOcean) { map[i].distToOcean = steps[i]; }
        }
    }

    for (Cell& cell : map)
    {
        if (!cell.oceanBool && cell.distToOcean == 1) { cell.coastBool = true; }
    }
}

void smoothTemps(std::vector<Cell>& map, int smoothTimes)
{
    for (int j = 0; j < smoothTimes; j++)
    {
        for (int i = 0; i < map.size(); i++)
        {
            float temp = 0;
            for (int j = 0; j < map[i].neighbors.size(); j++)
            {
                temp += map[map[i].neighbors[j]].temp;
            }
            map[i].temp = temp / map[i].neighbors.size();
        }
    }
}

void calcHumid(std::vector<Cell>& map)
{
    // Humidity should depend on:
    // 1. Temperature (warmer air holds more moisture)
    // 2. Precipitation (more rain = more moisture available)
    // 3. Proximity to water bodies
    // 4. Evapotranspiration from vegetation
    for (int i = 0; i < map.size(); i++)
    {
        float humidity = 0.0f;

        if (map[i].oceanBool)
        {
			// Ocean cells: very high humidity
            // Warmer water = more evaporation
			float tempFactor = clamp((map[i].temp + 10.0f) / 40.0f, 1.0f, 0.1f);
			humidity = 0.8 + 0.2f * tempFactor;
        }
        else
        {
            // Base humidity from precipitation and temperature
            // Logarithmic realtionship: more precip = higher humidity, but with diminishing returns
			float precipFactor = std::log(map[i].percepitation + 1.0f) / std::log(500.0f + 1.0f); // assuming 500mm/year max
			precipFactor = clamp(precipFactor, 1.0f, 0.0f);

            // Temperature effect on stauration capacity
			// warmer air holds more moisture, but also increases evapotranspiration
			float tempFactor = 1.0f / (1.0f + std::exp(-0.1f * (map[i].temp - 15.0f))); // sigmoid centered at 15C

			// Distance to ocean effect, coastal areas are more humid
			float oceanProximityFactor = std::exp(-0.03f * map[i].distToOcean); // decays with distance

            // Vegetation/surface moisture effect (simplified, should link to biome later)
			float surfaceMoisture = map[i].treeBool ? 0.1f : 0.0f; // more vegetation = more evapotranspiration

			// Combine factors
            humidity = precipFactor * 0.5f // 50% for precipitation
                     + tempFactor * 0.3f      // 30% for temperature
                     + oceanProximityFactor * 0.15f // 15% for ocean proximity
				+ surfaceMoisture * 0.05f; // 5% for surface moisture

			// Height adjustment: higher altitudes tend to be less humid
            if (map[i].height > 0.7f)
            {
				humidity *= (1.0f - (map[i].height - 0.7f) * 0.5f); // reduce humidity at high altitudes
            }
			humidity = clamp(humidity, 1.0f, 0.0f);
        }
		map[i].humidity = humidity;
    }
}

//////////
// Sphere versions of the position based steps
//////////

void setRandomEulerPole(Continent& continent)
{
    // Uniform height and azimuth give a uniformly random direction on the sphere
    const float z = RandomBetween(-1.0f, 1.0f);
    const float azimuth = RandomBetween(0.0f, 2.0f * glm::pi<float>());
    const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    continent.eulerAxis = glm::vec3(r * std::cos(azimuth), z, r * std::sin(azimuth));
    continent.angularSpeed = RandomBetween(PLATE_SPEED_MIN, PLATE_SPEED_MAX);
}

void planetContinentGeneration(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config)
{
    std::vector<Cell>& map = planet.cells;
    const int npeaks = static_cast<int>(config.npeaks);

    // reserve space for continents
    globals.continents.reserve(npeaks);
    for (int i = 0; i < npeaks; i++)
    {
        globals.continents.emplace_back(Continent(i));
    }
    // Prepare active queues for continent assignment
    std::vector<std::vector<std::size_t>> active(npeaks);
    std::vector<bool> assigned(map.size(), false);
    std::vector<std::size_t> seeds(npeaks);
    std::size_t size = map.size();
    std::size_t assigned_cells = 0;

    // Assign seed cells to each continent
    for (int i = 0; i < npeaks; i++)
    {
        std::size_t index = rand_long() % size;
        while (assigned[index])
        {
            index = rand_long() % size;
        }
        assigned[index] = true;
        assigned_cells++;
        seeds[i] = index;
        map[index].continent = i;
        globals.continents[i].addCell(index);
        active[i].push_back(index);

        // add neighbors to active
        for (int j = 0; j < map[index].neighbors.size(); j++)
        {
            std::size_t neighborIdx = map[index].neighbors[j];
            if (!assigned[neighborIdx])
            {
                active[i].push_back(neighborIdx);
            }
        }
    }

    // Grow continents until all cells are assigned
    while (assigned_cells < size)
    {
        std::size_t continetIdx = rand_long() % npeaks;
        if (active[continetIdx].empty()) {
            continue;
        }
        // Get next cell from the continent's active queue
        std::size_t cellIdx = pop_random_i(active[continetIdx]);
        if (assigned[cellIdx]) {
            continue;
        }
        assigned[cellIdx] = true;
        assigned_cells++;
        map[cellIdx].continent = continetIdx;
        globals.continents[continetIdx].addCell(cellIdx);
        // add neighbors to active
        for (int j = 0; j < map[cellIdx].neighbors.size(); j++)
        {
            std::size_t neighborIdx = map[cellIdx].neighbors[j];
            if (!assigned[neighborIdx])
            {
                active[continetIdx].push_back(neighborIdx);
            }
        }
    }
    // clean up continents
    if (assigned_cells != size)
    {
        for (std::size_t i = 0; i < map.size(); i++)
        {
            if (map[i].continent == -1)
            {
                // Find nearest continent by checking neighbors
                for (std::size_t j = 0; j < map[i].neighbors.size(); j++)
                {
                    int neighborContinent = map[map[i].neighbors[j]].continent;
                    if (neighborContinent != -1)
                    {
                        map[i].continent = neighborContinent;
                        globals.continents[neighborContinent].addCell(i);
                        break;
                    }
                }

                // If still no continent, take the one whose seed is closest. The flat version measured from
                // voronoi_points[i] (a corner index used as a cell index) to centres that were not computed yet
                if (map[i].continent == -1)
                {
                    float minAngle = std::numeric_limits<float>::max();
                    int closestContinent = 0;

                    for (int j = 0; j < npeaks; j++)
                    {
                        float angle = angleBetween(planet.points[i], planet.points[seeds[j]]);
                        if (angle < minAngle)
                        {
                            minAngle = angle;
                            closestContinent = j;
                        }
                    }

                    map[i].continent = closestContinent;
                    globals.continents[closestContinent].addCell(i);
                }
            }
        }
    }
    // Create the plates
    for (int i = 0; i < npeaks; i++)
    {
        setRandomEulerPole(globals.continents[i]);
        globals.continents[i].setAge(RandomBetween(0.5f, 1.0f)); // Age in billions of years
        globals.continents[i].calcCenterOnSphere(planet.points);

        globals.continents[i].setHeight(RandomBetween(0.3f, 0.7f)); // Initial height (is replaced later)
    }
}

void planetContinentInteraction(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, int propagation_depth)
{
    // Get the interactions between the continents
    /*
    1. Determine boundry cells
    2. Compare the plate velocities at the cell along the local boundary normal and add height
    3. Propagate
    */
    std::vector<Cell>& map = planet.cells;

    for (std::size_t cellIndex = 0; cellIndex < map.size(); cellIndex++)
    {
        Cell& cell = map[cellIndex];
        int mainContinentId = cell.continent;
        if (mainContinentId < 0 || mainContinentId >= static_cast<int>(globals.continents.size())) {
            continue;
        }

        bool isBoundry = false;
        std::vector<int> neighboringContinents;

        for (int neighborIdx : cell.neighbors)
        {
            int neighborContinent = map[neighborIdx].continent;
            if (neighborContinent >= 0 && neighborContinent != cell.continent
                && std::find(neighboringContinents.begin(), neighboringContinents.end(), neighborContinent) == neighboringContinents.end())
            {
                neighboringContinents.push_back(neighborContinent);
                isBoundry = true;
            }
        }
        if (!isBoundry)
        {
            continue;
        }
        // add to the boundry list of the main continent
        globals.continents[mainContinentId].addBoundryCell(cellIndex);

        Continent& mainContinent = globals.continents[mainContinentId];
        const glm::vec3 p = planet.points[cellIndex];
        const glm::vec3 mainVelocity = mainContinent.velocityAt(p);

        float totalHeightIncrease = 0.0f;
        bool hasVolcanicActivity = false;
        BoundaryType dominantBoundaryType = BoundaryType::None;

        for (int neighborContinentId : neighboringContinents)
        {
            Continent& neighborContinent = globals.continents[neighborContinentId];

            // Boundary normal points from the main plate to the neighbour plate. It is local: the tangent part
            // of the directions to this cell's neighbours on that plate (the flat map used the line between
            // the two plate centres, the same for the whole boundary)
            glm::vec3 towardNeighbor(0.0f);
            for (int neighborIdx : cell.neighbors)
            {
                if (map[neighborIdx].continent == neighborContinentId) {
                    towardNeighbor += planet.points[neighborIdx] - p;
                }
            }
            glm::vec3 boundaryNormal = towardNeighbor - glm::dot(towardNeighbor, p) * p;
            float normalMagnitude = glm::length(boundaryNormal);
            if (normalMagnitude < 1e-6f) {
                continue;
            }
            boundaryNormal /= normalMagnitude;

            // Relative motion of the two plates at this point
            const glm::vec3 relativeVelocity = mainVelocity - neighborContinent.velocityAt(p);

            // Project relative velocity onto boundary normal
            // Positive = converging, Negative = diverging, Zero = transform
            float convergenceRate = glm::dot(relativeVelocity, boundaryNormal);
            float tangentialRate = std::abs(glm::dot(relativeVelocity, glm::cross(p, boundaryNormal)));

            // Determine boundary type
            BoundaryType boundaryType;

            // Transform if tangentialRate is significantly higher than convergenceRate
            if (tangentialRate > std::abs(convergenceRate) * 1.5f && tangentialRate > 0.2f) {
                boundaryType = BoundaryType::Transform;
            }
            else if (convergenceRate > 0.2f) { // Converging
                if (mainContinent.plateType == PlateType::Continental &&
                    neighborContinent.plateType == PlateType::Continental) {
                    boundaryType = BoundaryType::ContinentalCollision;
                }
                else {
                    boundaryType = BoundaryType::Subduction;
                }
            }
            else if (convergenceRate < -0.2f) { // Diverging
                if (mainContinent.plateType == PlateType::Oceanic &&
                    neighborContinent.plateType == PlateType::Oceanic) {
                    boundaryType = BoundaryType::OceanicRift;
                }
                else if (mainContinent.plateType == PlateType::Continental &&
                    neighborContinent.plateType == PlateType::Continental) {
                    boundaryType = BoundaryType::ContinentalRift;
                }
                else {
                    boundaryType = BoundaryType::OceanicRift;
                }
            }
            else {
                boundaryType = BoundaryType::None;
            }

            if (dominantBoundaryType == BoundaryType::None)
            {
                dominantBoundaryType = boundaryType;
            }
            // Calculate height modifier based on boundary type
            float collisionIntensity = std::min(std::abs(convergenceRate) - 0.2f, 1.0f);
            float heightModifier = calculateBoundaryHeightModifier(boundaryType, convergenceRate,
                mainContinent.getAge(), neighborContinent.getAge());

            heightModifier *= (0.25f * collisionIntensity);

            switch (boundaryType)
            {
            case BoundaryType::ContinentalCollision:
            {
                // Himmalaya style
                heightModifier += RandomBetween(0.05f, 0.15f) * collisionIntensity;
                // Higher chance of volcanic activity in younger continents
                if (mainContinent.getAge() < 0.7f && RandomBetween(0.0f, 1.0f) < 0.3f * collisionIntensity)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            case BoundaryType::Subduction:
            {
                // Subduction zones like Andes
                bool isMainOceanic = mainContinent.plateType == PlateType::Oceanic;
                if (!isMainOceanic)
                {
                    // Continent side gets volcanic mountains
                    heightModifier += RandomBetween(0.08f, 0.18f);

                    // Higher chance of volcanic activity
                    if (RandomBetween(0.0f, 1.0f) < 0.6f)
                    {
                        hasVolcanicActivity = true;
                    }
                }
                else
                {
                    // Oceanic side gets deep trenches
                    heightModifier -= RandomBetween(0.05f, 0.12f);
                }
                break;
            }
            case BoundaryType::OceanicRift:
            {
                // Mid-ocean ridges
                heightModifier += RandomBetween(0.02f, 0.08f);
                // moderate volcanic activity
                if (RandomBetween(0.0f, 1.0f) < 0.3f)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            case BoundaryType::ContinentalRift:
            {
                // continental rifts valley formation
                heightModifier -= RandomBetween(0.05f, 0.12f);

                // minor volcanic activity
                if (RandomBetween(0.0f, 1.0f) < 0.15f)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            case BoundaryType::Transform:
            {
                // Minor height changes
                heightModifier += RandomBetween(-0.02f, 0.02f);
                // slight chance of volcanic activity
                if (RandomBetween(0.0f, 1.0f) < 0.05f)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            default:
                break;
            }
            totalHeightIncrease += heightModifier;
        }
        // Apply total height increase to the cell - no clamping
        cell.height += totalHeightIncrease;
        if (hasVolcanicActivity) {
            cell.volcanicActivity = true;
            cell.height += RandomBetween(0.08f, 0.2f); // Allow tall volcanic peaks
        }

        // Propagation of mountain building effects
        propagateTerrainFeatures(map, globals, cellIndex, totalHeightIncrease, hasVolcanicActivity, dominantBoundaryType, propagation_depth);
    }
}

void planetSimplexNoise(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config)
{
    // Generate a layer of simplex noise and add it to the height
    int seed = rand();
    SimplexNoise simplex(seed);

    // Configure noise parameters (SHOULD BE IN CONFIG)
    const int octaves = 3;         // Number of frequency layers (more = more detail)
    const float persistence = 0.5f; // How much each octave contributes (0-1)
    const float noiseStrength = 0.3f; // How much the noise affects the height (0-1)

    // Apply noise to all cells, sampled in 3D so it has no seam or pole
    for (std::size_t i = 0; i < planet.cells.size(); i++)
    {
        const glm::vec3 p = planet.points[i] * NOISE_FREQUENCY;
        float noiseValue = simplex.octaveNoise(p.x, p.y, p.z, octaves, persistence);

        // convert noise to [0,1]
        noiseValue = (noiseValue + 1.0f) * 0.5f;

        // Keep noise centered around 0 for natural variation
        noiseValue = noiseValue * 0.5f; // Range: [0, 0.5]

        // Apply noise - no clamping, let heights be natural
        planet.cells[i].height += noiseValue * noiseStrength;
    }
}

void planetNoiseHeight(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config, int iterations)
{
    std::vector<Cell>& map = planet.cells;
    if (config.heigth_noise_method == 1)
    {
        size_t size = map.size();
        for (int i = 0; i < iterations; i++)
        {
            for (size_t j = 0; j < size; j++)
            {
                map[j].height = map[j].height + RandomBetween(-0.005, 0.005);
            }
        }
    }
    else if (config.heigth_noise_method == 2)
    {
        planetSimplexNoise(planet, globals, config);
    }
    else
    {
        throw std::invalid_argument("Invalid method for noise height generation");
    }
}

// MEthod should be enum or something
// 1 = k-peaks, 2 = k-peaks continet, 3 = Continental, 4 = Continental 2
// k-point smooth height generator
// there is a max of RAND_MAX_LONG (about a million cells)
void planetHeightGen(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config)
{
    std::vector<Cell>& map = planet.cells;

    if (config.height_method < 3)
    {
        // Method 1 is without continent interactions and 2 is with.
        /*
        1. Initiate queue active
        2. Pick k random starting cells
        3. Assign height above 0.9 to these cells and add their neighbors to active
        4.  Check neighbor's height
            if not 0 save it else add to active if not already there
            add small probability of height increase for each neighbor
        5. Calculate neighbor averages and add random factor
        6. Go back to step 4 until active is empty
        */
        // Active cells that have not been assigned a height yet, should be a queue of some sort
        std::vector<std::size_t> active;
        active.reserve(map.size() - 1);
        int numOceanContinents = static_cast<int>(config.npeaks * config.oceanic_plate_ratio);

        for (int i = 0; i < static_cast<int>(config.npeaks); i++)
        {
            std::size_t index = rand_long() % map.size();

            // Select if the peak should be oceanic or continental
            if (RandomBetween(0.0f, 1.0f) < 0.1f)
            {
                globals.continents.emplace_back(Continent(i));
                globals.continents[i].plateType = PlateType::Mixed;
                map[index].height = normalDistPDF(0.5, 0.2);
                numOceanContinents--;
            }
            if (numOceanContinents > 0)
            {
                globals.continents.emplace_back(Continent(i));
                globals.continents[i].plateType = PlateType::Oceanic;
                map[index].height = normalDistPDF(0.3, 0.1);
                numOceanContinents--;
            }
            else
            {
                globals.continents.emplace_back(Continent(i));
                globals.continents[i].plateType = PlateType::Continental;
                map[index].height = normalDistPDF(0.7, 0.2);
            }

            active.insert(std::end(active), std::begin(map[index].neighbors), std::end(map[index].neighbors));

            // Continents
            globals.continents[i].addCell(index);
            map[index].continent = i;
            globals.continents[i].centerDir = planet.points[index]; // The centre is the first cell added

            globals.continents[i].setAge(RandomBetween(0.5, 0.9));
            setRandomEulerPole(globals.continents[i]);
            globals.continents[i].setHeight(map[index].height);
            if (globals.continents[i].plateType == PlateType::Oceanic)
            {
                globals.continents[i].crustThickness = RandomBetween(
                    config.oceanic_crust_thickness * 0.8f,
                    config.oceanic_crust_thickness * 1.2f
                );
                globals.continents[i].baseDensity = RandomBetween(
                    0.8f,
                    1.2f
                );
            }
            else
            {
                globals.continents[i].crustThickness = RandomBetween(
                    config.continental_crust_thickness * 0.8f,
                    config.continental_crust_thickness * 1.2f
                );
                globals.continents[i].baseDensity = RandomBetween(
                    0.8f,
                    1.2f
                );
            }

            // add the actually inserted neighbors into continent
            for (std::size_t neighbor = 0; neighbor < map[index].neighbors.size(); neighbor++)
            {
                globals.continents[i].addCell(map[index].neighbors[neighbor]);
                map[map[index].neighbors[neighbor]].continent = i;
            }
        }

        while (active.empty() == false)
        {
            std::size_t index = pop_random_i(active);

            float height_sum = 0.0;
            int count_values = 0;

            for (std::size_t j = 0; j < map[index].neighbors.size(); j++)
            {
                size_t neighborIdx = map[index].neighbors[j];
                // small probability of random height increase, THIS is heavily up to tuning for interesting maps
                // Also should be reconsidered
                if (RandomBetween(0.0, 1.0) < config.prob_of_island && height_sum < config.dist_from_mainland && count_values > 1)
                {
                    map[neighborIdx].height = normalDistPDF(0.7, 0.3);
                    active.insert(std::begin(active), std::begin(map[neighborIdx].neighbors), std::end(map[neighborIdx].neighbors));

                    for (std::size_t k = 0; k < map[neighborIdx].neighbors.size(); k++)
                    {
                        globals.continents[map[index].continent].addCell(map[neighborIdx].neighbors[k]);
                        map[map[neighborIdx].neighbors[k]].continent = map[index].continent;
                    }
                }
                // check if neighbor has height
                if (map[neighborIdx].height != 0.f)
                {
                    height_sum = height_sum + map[neighborIdx].height;
                    count_values++;
                }
                else
                {
                    // Checks for duplicates then adds to active
                    if (std::find(active.begin(), active.end(), map[index].neighbors[j]) == active.end()) {
                        active.push_back(map[index].neighbors[j]);
                        // Add the neighbors to the continent as well
                        globals.continents[map[index].continent].addCell(map[index].neighbors[j]);
                        map[neighborIdx].continent = map[index].continent;
                    }
                }
            }
            if (count_values == 0) { active.push_back(map[index].id); }
            else {
                map[index].height = (height_sum / count_values) + RandomBetween(-config.delta_max_neg, config.delta_max_pos);
            }
        }
        // Fill out missing continent values
        for (std::size_t i = 0; i < map.size(); i++)
        {
            if (map[i].continent == -1)
            {
                // check neighbors and find the first non missing
                for (std::size_t j = 0; j < map[i].neighbors.size(); j++)
                {
                    if (map[map[i].neighbors[j]].continent != -1)
                    {
                        map[i].continent = map[map[i].neighbors[j]].continent;
                        break;
                    }
                }
            }
        }

        if (config.height_method == 2)
        {
            planetContinentInteraction(planet, globals);
        }
        cleanUpContinents(map, globals);

        rise(map); // calculate the rise of the map with the new height values

        simulateErosion(map, globals, config); // simulate erosion on the height values
        planetSimplexNoise(planet, globals, config);
        rise(map); // calculate the rise of the map with the new height values
    }
    else if (config.height_method == 3)
    {
        // Method 3 is a continental generation method
        /*
        1. Initiate K queues for the continents
        2. Initiate a list of cells for each continent
        3. Pick K random starting cells
        4. Randomly pick a continent and add the neighbors to the active queue
        5. Repeat until all cells are assigned
        6. Assign a height to each continent then add noise filter (simplex?) and add to each cell in the continent
        7. Assign directions for continental drift
        8. Calculate the interactions between the continents
        9. Calculate the rise of the map with the new height values
        */

        planetContinentGeneration(planet, globals, config);

        // Calculate the interactions between the continents
        planetContinentInteraction(planet, globals);

        rise(map); // calculate the rise of the map with the new height values

        smooth_height(map, config.rise_threshold, 1, config.smooth_method); // smooth the height values

        // Overlay a noise filter (simplex noise)
        planetSimplexNoise(planet, globals, config);

        // The flat map built the plate boundary lines here, on the sphere they come back with the overlays

        rise(map); // calculate the rise of the map with the new height values
    }
    else if (config.height_method == 4)
    {
        // Method 4 is an improved continental generation method
        planetContinentGeneration(planet, globals, config);
        assignPlateTypes(globals.continents, config);
        setBaseElevations(map, globals);
        planetContinentInteraction(planet, globals);
        simulateErosion(map, globals, config);
        planetSimplexNoise(planet, globals, config);
        rise(map); // calculate the rise of the map with the new height values
        smooth_height(map, config.rise_threshold, 1, config.smooth_method); // smooth the height values
        rise(map); // calculate the rise of the map with the new height values
    }
}

//////////
// Climate on the sphere
//////////

float latToMapFraction(float lat)
{
    return (90.0f - lat) / 180.0f;
}

float climateEquatorFraction(const GlobalWorldObjects& globals)
{
    if (globals.convergenceLines.empty()) { return 0.5f; }
    return globals.convergenceLines[globals.convergenceLines.size() / 2];
}

float normalizedClimateLatitude(float lat, float equatorFraction)
{
    // The flat map divided the distance from the equator by half the map height
    return std::min(std::abs(latToMapFraction(lat) - equatorFraction) / 0.5f, 1.0f);
}

void planetCalcWind(vor::SphereVoronoi& planet, GlobalWorldObjects& globals)
{
    std::vector<Cell>& map = planet.cells;

    // Wind is using simplex noise to generate variation along the convergence lines

    // Generate noise for wind variation
    int windSpeed = rand();
    SimplexNoise noiseWind(windSpeed);

    // Wind parameters
    const float noiseAmplitude = 45.0f;  // Amplitude of the noise in degrees
    const int octaves = 2;               // detail levels
    const float persistence = 0.6f;      // amplitude decrease per octave

    const std::vector<float>& lines = globals.convergenceLines;
    const int numLines = static_cast<int>(lines.size());
    const int lastZone = static_cast<int>(globals.windDirection.size()) - 1;
    if (lastZone < 0 || globals.windStrength.size() != globals.windDirection.size()) { return; }

    for (size_t i = 0; i < map.size(); i++)
    {
        const float lat = planet.cellLat[i];
        const float t = latToMapFraction(lat);

        // The zone is the number of lines north of the cell, zone 0 is the north polar zone.
        // The flat map took the wind of the closest line's index instead, so half the cells got a neighbouring zone's wind
        int zone = 0;
        float min_dist = 1000000.f;
        for (int j = 0; j < numLines; j++)
        {
            if (lines[j] < t) { zone++; }
            min_dist = std::min(min_dist, std::abs(t - lines[j]));
        }
        zone = std::min(zone, lastZone);

        // Base wind from zone
        float baseDir = globals.windDirection[zone];
        float baseStr = globals.windStrength[zone];

        // Distance from convergence line affects strength, measured in line spacings
        float distFromLine = numLines > 0 ? min_dist * numLines : 1.f;
        distFromLine = clamp(distFromLine, 1.f, 0.f);

        // strength varies with from zone center
        float strengthModifier = 0.8f + 0.2f * (1.0f - distFromLine);

        // Generate noise variation for direction
        const glm::vec3 p = planet.points[i] * WIND_NOISE_FREQUENCY;
        float noiseValue = noiseWind.octaveNoise(p.x, p.y, p.z, octaves, persistence);

        // Apply noise to create natural wind variation
        float windDir = noiseValue * noiseAmplitude;

        // Coriolis-like effect: wind curves near poles
        float latitudeFactor = std::abs(lat) / 90.0f; // 0 at equator, 1 at poles
        float coriolisDeflection = latitudeFactor * 15.0f * (lat < 0.0f ? 1.0f : -1.0f);

        // Combining direction components
        float finalDir = normalizeAngle(baseDir + windDir + coriolisDeflection);

        // Terrain effects on wind strength (mountains block wind) (can be improved)
        float terrainFactor = 1.0f;
        if (map[i].height > 0.75f)
        {
            terrainFactor = 1.0f - (map[i].height - 0.6f) * 0.7f; // reduce strength in high terrain
            terrainFactor = std::max(terrainFactor, 0.3f);
        }
        // Ocean vs land: ocean has stronger more consistent winds
        float surfaceFactor = map[i].oceanBool ? 1.05f : 0.95f;

        // Calculate final wind strength
        float finalStr = baseStr * strengthModifier * terrainFactor * surfaceFactor;
        finalStr = clamp(finalStr + RandomBetween(-0.1f, 0.1f), 1.f, 0.05f);

        map[i].windDir = finalDir;
        map[i].windStr = finalStr;
    }

    // Smoothing winds to create coherent patterns
    // Average with neighbors. Each wind is a tangent vector at its own cell, the sum is read back
    // as a bearing in this cell's tangent plane, which also works next to the poles
    std::vector<glm::vec3> windVectors(map.size());
    for (size_t i = 0; i < map.size(); i++)
    {
        windVectors[i] = bearingToTangent(planet.points[i], map[i].windDir);
    }

    std::vector<float> smoothedDirs(map.size(), 0.f);
    std::vector<float> smoothedStrs(map.size(), 0.f);

    for (size_t i = 0; i < map.size(); i++)
    {
        glm::vec3 sum = windVectors[i] * 2.0f;
        float sumStr = map[i].windStr * 2.0f;
        int count = 2;
        for (int neighborIdx : map[i].neighbors)
        {
            sum += windVectors[neighborIdx];
            sumStr += map[neighborIdx].windStr;
            count++;
        }
        smoothedDirs[i] = tangentToBearing(planet.points[i], sum);
        smoothedStrs[i] = sumStr / count;
    }
    // Apply smoothed values
    for (size_t i = 0; i < map.size(); i++)
    {
        map[i].windDir = smoothedDirs[i];
        map[i].windStr = smoothedStrs[i];
    }
}

void planetCalcTemp(vor::SphereVoronoi& planet, GlobalWorldObjects& globals)
{
    std::vector<Cell>& map = planet.cells;

    // Find actual equator
    const float equatorFraction = climateEquatorFraction(globals);

    // Planetary temperature modifiers
    float baseTemp = globals.globalTempAvg;
    float equatorToPoleGradient = globals.planetaryParams.equatorToPoleTemp;
    float greenhouseBoost = globals.planetaryParams.greenhouseEffectFactor;
    float axisTilt = globals.planetaryParams.axialTilt;

    float atmosphericRetention = std::sqrt(globals.planetaryParams.atmosphere.totalPressure);

    for (int i = 0; i < map.size(); i++)
    {
        float temp = 0.f;

        // 1. Latitute based on equator
        float normalizedLatitude = normalizedClimateLatitude(planet.cellLat[i], equatorFraction);

        // Temperature decrease from equator to pole on planetary gradient
        float latTemp = baseTemp - (equatorToPoleGradient * std::pow(normalizedLatitude, 1.3f));
        temp += latTemp;

        // 2. Altitude modifier
        // Temp decreases with altitude (standard lapse rate ~6.5 C per km, scaled by atmospheric retention)
        // ocean depth increases temp slightly
        if (map[i].height >= globals.seaLevel)
        {
            // Land: standard lapse rate scaled by atmospheric retention
            float lapseRate = 6.5f / atmosphericRetention; // C per km
            float elevationKm = (map[i].height - globals.seaLevel) * 10.0f; // in km
            temp -= clamp(lapseRate * elevationKm, 70.0f, -50.0f);
        }
        else
        {
            // Ocean: slight increase in temp with depth
            float lapseRate = 6.5f / (atmosphericRetention * 5.0f); // Reduced lapse rate for ocean depth
            float depth = (globals.seaLevel - map[i].height) * 5.0f; // in km
            temp -= lapseRate * depth;
        }

        // 3. Distance from ocean (continental effect)
        // land far away from ocean is more extreme
        // stronger on less atmosphere circulation
        float continentalityFactor = 1.0f - std::abs(normalizedLatitude - 0.5f) * 2.0f; // Strongest at mid-latitudes
        // Ocean cells use distToOcean for their distance to land, the continental effect is only for land
        float oceanProximityEffect = map[i].oceanBool ? 0.f : map[i].distToOcean * 0.3f * continentalityFactor;

        // Effect depends on latitude and atmospheric retention
        float latitudeFactor = 1.0f - std::abs(normalizedLatitude - 0.5f) * 2.0f; // Strongest at mid-latitudes
        temp += oceanProximityEffect * latitudeFactor;

        // 4. Axial tilt
        // higher tilt = more seasonal variation
        // for now we just add a small modifier based on latitude
        float tiltEffect = (axisTilt / 23.5f) * normalizedLatitude * 3.0f;
        temp += RandomBetween(-tiltEffect, tiltEffect);

        // 5. Atmospheric Composition
        // Greenhouse effect
        temp += greenhouseBoost * (1.0f - normalizedLatitude * 0.3f);

        // Preassure effects heat retention
        if (globals.planetaryParams.atmosphere.totalPressure < 0.5f)
        {
            // Thin atmosphere: extreme day/night temperature swings (add randomness
            temp += RandomBetween(-10.0f, 10.0f) * (0.5f - globals.planetaryParams.atmosphere.totalPressure);
        }

        map[i].temp = temp;
    }
}

void planetCalcPercepitation(vor::SphereVoronoi& planet, GlobalWorldObjects& globals, const MapConfig& config)
{
    std::vector<Cell>& map = planet.cells;

    const int runs = static_cast<int>(config.percepitation_repeats);
    const float max_percipitation = config.max_percipitation;
    const float ocean_evaporation_factor = config.ocean_evaporation_factor;
    const float land_evapotranspiration_factor = config.land_evapotranspiration_factor;
    const float moisture_loss_rate = config.moisture_loss_rate;
    const float orographic_factor = config.orographic_factor;
    // The flat map passed moisture_loss_rate here, so config.condensation_rate has never been used. Kept for now
    const float condensation_rate = config.moisture_loss_rate;

    // Alogrithm Overview:
    // 1. For each cell, calculate initial moisture based on evaporation (higher over oceans, modulated by temperature and vegetation)
    // 2. For a number of advection steps:
    //    a. For each cell, determine the best neighbor aligned with wind direction
    //    b. Move moisture to that neighbor based on wind strength
    //    c. Calculate precipitation based on moisture, temperature, and terrain features (orographic lift)
    //   d. Reduce moisture by precipitation and natural loss
    // 3. Repeat for multiple runs to simulate seasonal variations
    // 4. Average the precipitation over all runs for final value
    // 5. Clamp and smooth the final precipitation values

    for (int run = 0; run < runs; run++)
    {
        std::vector<float> moisture(map.size(), 0.0f);
        std::vector<float> newPrecipitation(map.size(), 0.0f);

        // Step 1: Generate initial moisture from evaporation
        #pragma omp parallel for schedule(static)
        for (std::size_t i = 0; i < map.size(); i++)
        {
            float evaporation = 0.0f;

            if (map[i].oceanBool)
            {
                float tempFactor = clamp((map[i].temp + 20.0f) / 50.0f, 1.0f, 0.1f);
                evaporation = ocean_evaporation_factor * tempFactor * 100.0f;
            }
            else
            {
                float tempFactor = clamp((map[i].temp + 10.0f) / 40.0f, 1.0f, 0.1f);
                float vegetationFactor = map[i].treeBool ? 1.2f : 0.7f;
                evaporation = land_evapotranspiration_factor * tempFactor * vegetationFactor * 25.0f;
            }
            moisture[i] = evaporation;
        }

        // Step 2: Moisture advection with precipitation
        const int advectionSteps = 50;

        // Vary the processing order to avoid bias
        std::vector<std::size_t> cellOrder(map.size());
        for (std::size_t i = 0; i < map.size(); i++) {
            cellOrder[i] = i;
        }
        if (run > 0) {
            std::shuffle(cellOrder.begin(), cellOrder.end(), std::default_random_engine(rand()));
        }

        // Precompute wind direction and strength for each cell to optimize performance
        struct WindData {
            float windStr;
            int bestNeighbor;
            float bestAlignment;
        };
        std::vector<WindData> windCache(map.size());

        #pragma omp parallel for schedule(static)
        for (std::size_t idx = 0; idx < map.size(); idx++)
        {
            float windVariance = std::sin(run * 2.0f) * 20.f; // +/- 20 degrees
            const glm::vec3& p = planet.points[idx];
            const glm::vec3 wind = bearingToTangent(p, map[idx].windDir + windVariance);

            // The neighbour best lined up with the wind, by the cosine between the wind and the
            // direction to the neighbour in this cell's tangent plane
            int bestNeighbor = -1;
            float bestAlignment = -1.0f;
            for (int neighborIdx : map[idx].neighbors)
            {
                if (neighborIdx < 0 || neighborIdx >= map.size()) continue;
                glm::vec3 toNeighbor = planet.points[neighborIdx] - p;
                toNeighbor -= glm::dot(toNeighbor, p) * p;
                const float length = glm::length(toNeighbor);
                if (length < 1e-9f) continue;
                float alignment = glm::dot(toNeighbor / length, wind);
                if (alignment > bestAlignment)
                {
                    bestAlignment = alignment;
                    bestNeighbor = neighborIdx;
                }
            }
            windCache[idx] = { map[idx].windStr, bestNeighbor, bestAlignment };
        }


        for (int step = 0; step < advectionSteps; step++)
        {
            std::vector<float> newMoisture(map.size(), 0.0f);

            for (std::size_t idx = 0; idx < cellOrder.size(); idx++)
            {
                std::size_t i = cellOrder[idx];

                if (moisture[i] < 0.01f) continue;

                float cellMoisture = moisture[i];
                const WindData& windData = windCache[i];

                // Calculate precipitation
                float precipitationAmount = 0.0f;

                // 1. Baseline natural condensation (happens everywhere with sufficient moisture)
                if (cellMoisture > 10.0f)  // Only if there's enough moisture
                {
                    float moistureFactor = std::min(cellMoisture / 100.0f, 1.0f);
                    float baseCondensation = condensation_rate * moistureFactor * 12.0f;

                    // Vary ocndensation rate per run to simulate seasonal changes
                    float seasonalFactor = 0.8f + 0.4f * (run / static_cast<float>(runs));

                    precipitationAmount += baseCondensation * seasonalFactor;
                }

                // 2. Temperature-driven condensation (colder air holds less moisture)
                if (map[i].temp < 20.0f && cellMoisture > 20.0f)
                {
                    float coldFactor = clamp((20.0f - map[i].temp) / 30.0f, 1.0f, 0.0f);  // Stronger at colder temps
                    precipitationAmount += cellMoisture * 0.08f * coldFactor;
                }

                // 3. Orographic precipitation (mountains force uplift)
                if (windData.bestNeighbor >= 0 && !map[i].oceanBool)  // Only over land
                {
                    float heightDiff = map[windData.bestNeighbor].height - map[i].height;
                    if (heightDiff > 0.015f)  // Lower threshold for gentler slopes
                    {
                        float liftFactor = std::tanh(orographic_factor * heightDiff);
                        precipitationAmount += cellMoisture * liftFactor * 0.35f;
                    }
                }

                // 4. Oceanic storm precipitation (based on moisture and wind)
                if (map[i].oceanBool && cellMoisture > 50.0f && windData.windStr > 0.6f)
                {
                    float stormFactor = (windData.windStr - 0.6f) * 2.5f;  // Stronger winds = more storms
                    precipitationAmount += cellMoisture * 0.06f * stormFactor;
                }

                // Cap precipitation to avoid depleting all moisture
                precipitationAmount = std::min(precipitationAmount, cellMoisture * 0.65f);

                // Add precipitation
                newPrecipitation[i] += precipitationAmount;

                // Remaining moisture after precipitation
                float remainingMoisture = cellMoisture - precipitationAmount;

                // Transfer moisture to downwind neighbor
                if (windData.bestNeighbor >= 0 && windData.bestAlignment > 0.0f)
                {
                    // Transfer efficiency based on wind and alignment
                    // Vary transfer factor per run to simulate seasonal wind strength changes
                    float runVariation = 0.85 + 0.1f * std::cos(run * 1.5f);
                    float transferFactor = windData.windStr * std::pow(std::max(0.0f, windData.bestAlignment), 0.4f) * runVariation;
                    float transferredMoisture = remainingMoisture * transferFactor;

                    // Rain shadow effect (descending terrain)
                    if (!map[i].oceanBool)  // Only apply to land
                    {
                        float heightDiff = map[windData.bestNeighbor].height - map[i].height;
                        if (heightDiff < -0.02f)
                        {
                            transferredMoisture *= 0.70f;  // Rain shadow dries air
                        }
                    }

                    // Natural moisture loss during transport
                    transferredMoisture *= (1.0f - moisture_loss_rate);

                    newMoisture[windData.bestNeighbor] += transferredMoisture;
                    newMoisture[i] += remainingMoisture * (1.0f - transferFactor);
                }
                else
                {
                    // No good neighbor - moisture either precipitates or dissipates
                    newPrecipitation[i] += remainingMoisture * 0.25f;
                    newMoisture[i] += remainingMoisture * 0.75f;
                }
            }

            moisture = std::move(newMoisture);
        }

        // Step 3: Accumulate precipitation across runs
        #pragma omp parallel for schedule(static)
        for (std::size_t i = 0; i < map.size(); i++)
        {
            if (run == 0)
            {
                map[i].percepitation = newPrecipitation[i];
            }
            else
            {
                // Weight each run equally
                map[i].percepitation = (map[i].percepitation * run + newPrecipitation[i]) / (run + 1.0f);
            }
        }
    }
    // Step 4: Add tiny simplex noise for variability (needs a look)
    SimplexNoise simplexNoise = SimplexNoise(rand());
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
        const glm::vec3 p = planet.points[i] * PRECIP_NOISE_FREQUENCY;
        float noise = simplexNoise.noise(p.x, p.y, p.z) * 2.0f; // small variation
        map[i].percepitation = std::max(0.0f, map[i].percepitation + noise); // ensure non-negative
    }


    // Step 5: Cap the precipitation values
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
        map[i].percepitation = std::min(map[i].percepitation, max_percipitation);
    }

    // Step 6: Smooth precipitation for gradual transitions
    std::vector<float> smoothedPrecip(map.size());
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
        float sum = map[i].percepitation * 2.5f;  // Weight center
        int count = 2.5f;

        for (int neighborIdx : map[i].neighbors)
        {
            sum += map[neighborIdx].percepitation;
            count += 1;
        }

        smoothedPrecip[i] = sum / static_cast<float>(count);
    }

    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
        map[i].percepitation = smoothedPrecip[i];
    }
}

void planetCalcClimateVariance(vor::SphereVoronoi& planet, GlobalWorldObjects& globals)
{
    std::vector<Cell>& map = planet.cells;

    // Calculate Climate variance for each cell
    // 1. Ocean vs land
    // 2. coastal proximity
    // 3. latitude
    // 4. altitude
    // 5. atmospheric pressure

    const float equatorFraction = climateEquatorFraction(globals);
    float pressureFactor = 1.0f / std::sqrt(globals.planetaryParams.atmosphere.totalPressure + 0.1f);

    for (std::size_t i = 0; i < map.size(); i++)
    {
        Cell& cell = map[i];

        float normalizedLatitude = normalizedClimateLatitude(planet.cellLat[i], equatorFraction);

        // == Temperature variance ==
        float baseTempVar = 5.0f;

        // Ocean vs land
        if (cell.oceanBool)
        {
            baseTempVar *= 0.3f; // oceans have lower temp variance
        }
        else if (cell.coastBool)
        {
            baseTempVar *= 0.6f; // coastal areas have moderate temp variance
        }
        else
        {
            float continentalFactor = 1.0f + std::min(cell.distToOcean * 0.05f, 0.5f);
            baseTempVar *= continentalFactor;
        }
        // Latitude effect
        float latitudeVarFactor = 1.0f + normalizedLatitude * 0.8f;
        baseTempVar *= latitudeVarFactor;
        // Altitude effect
        if (cell.height > globals.seaLevel)
        {
            float altitudeFactor = 1.0f + (cell.height - globals.seaLevel) * 0.5f;
            baseTempVar *= altitudeFactor;
        }
        // Atmospheric pressure effect
        baseTempVar *= pressureFactor;
        cell.tempVariance = baseTempVar;

        // == Wind Variance ==
        float baseWindVar = 0.1f;
        // coastal effect
        if (cell.coastBool)
        {
            baseWindVar *= 1.5f;
        }
        // mid-latitudes tend to have more variable winds
        float midLatitudeFactor = 1.0f + (1.0f - std::abs(normalizedLatitude - 0.5f) * 2.0f) * 0.4f;
        baseWindVar *= midLatitudeFactor;

        cell.windStrVariance = baseWindVar;

        cell.windDirVariance = 15.0f + baseWindVar * 30.0f;

        // == Humidity Variance ==
        float baseHumidVar = 0.1f;
        // Dry areas have less humidity variance
        baseHumidVar *= (1.0f - cell.humidity) + 0.5f;
        // Coastal areas have less humidity variance
        if (cell.coastBool || cell.oceanBool)
        {
            baseHumidVar *= 0.7f;
        }
        cell.humidityVariance = baseHumidVar;

        // == Precipitation Variance ==
        // I set precip varinace to be a percentage of mean precip
        float basePrecipVar = 0.4f;
        // Tropical areas have higher precip variance
        if (normalizedLatitude < 0.3f)
        {
            basePrecipVar *= 1.5f;
        }
        // arid areas have higher precip variance
        if (cell.percepitation < 15.0f && cell.percepitation > 0.1f)
        {
            basePrecipVar *= 1.5f;
        }
        // Stable oceanic climate has lower precip variance
        if (cell.oceanBool)
        {
            basePrecipVar *= 0.6f;
        }
        cell.percepitationVariance = basePrecipVar;
    }
}
