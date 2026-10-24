#include "SphereBoundary.hpp"
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <cmath>
#include "Logger.h"

namespace {

    constexpr int MAX_CLAMP_PASSES = 10; // Halving passes for the fold-over guard, the last two set the strength to 0

    // Tolerances as fractions of the mean cell spacing. The flat map used 0.01 and 0.05 px with
    // cells about 7 px apart
    constexpr double DEGENERATE_FRACTION = 0.0015; // Shorter edges, or centres closer to the edge's great circle, have no reliable winding
    constexpr double SUBDIVIDE_FRACTION = 0.007;   // Shorter edges are not subdivided, their samples would be below float precision

    constexpr uint64_t FNV_OFFSET = 1469598103934665603ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;

    void hashMix(uint64_t& h, uint64_t value)
    {
        h ^= value;
        h *= FNV_PRIME;
    }

    int cellLabel(const Cell& cell, int type)
    {
        switch (static_cast<BorderType>(type))
        {
        case BorderType::Coast: return cell.oceanBool ? 1 : 0;
        case BorderType::Continent: return cell.continent;
        case BorderType::Biome: return cell.biome;
        default: return 0;
        }
    }

    // Positive when a, b, c run counter clockwise seen from outside the sphere
    double det(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c)
    {
        return glm::dot(c, glm::cross(a, b));
    }

    // Signed area of the spherical triangle a, b, c; positive when counter clockwise from outside
    double sphericalTriangleArea(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c)
    {
        return 2.0 * std::atan2(glm::dot(a, glm::cross(b, c)), 1.0 + glm::dot(a, b) + glm::dot(b, c) + glm::dot(c, a));
    }

    glm::dvec3 toDouble(const glm::vec3& v) { return glm::dvec3(v); }
}

void SphereBoundaries::invalidate()
{
    topologyValid = false;
    geometryValid = false;
}

bool SphereBoundaries::sync(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode)
{
    mapMode = std::clamp(mapMode, 0, MAP_MODE_COUNT - 1);

    if (!topologyValid || topoCells != planet.cells.size() || topoVoronoiPoints != planet.voronoi_points.size())
    {
        buildTopology(planet);
        geometryValid = false;
    }

    uint64_t key = computeKey(planet, settings, mapMode);
    if (geometryValid && key == geometryKey) { return false; }

    buildGeometry(planet, settings, mapMode);
    geometryKey = key;
    return true;
}

uint64_t SphereBoundaries::computeKey(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode) const
{
    // Labels can change without the planet changing (new biomes), so they are part of the key.
    // The mode itself is not, modes with the same active types share the geometry
    uint64_t h = FNV_OFFSET;
    hashMix(h, settings.geometryRevision);
    hashMix(h, planet.cells.size());
    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        if (!settings.modeTypes[mapMode][t]) { continue; }
        hashMix(h, static_cast<uint64_t>(t) + 1);
        for (const Cell& cell : planet.cells)
        {
            hashMix(h, static_cast<uint64_t>(static_cast<int64_t>(cellLabel(cell, t))));
        }
    }
    return h;
}

void SphereBoundaries::buildTopology(const vor::SphereVoronoi& planet)
{
    const std::size_t nCells = planet.cells.size();
    const std::size_t nVerts = planet.voronoi_points.size();

    const double spacing = nCells > 0 ? std::sqrt(4.0 * 3.14159265358979323846 / static_cast<double>(nCells)) : 0.0;
    degenerateTolerance = DEGENERATE_FRACTION * spacing;
    subdivideTolerance = SUBDIVIDE_FRACTION * spacing;

    vp.resize(nVerts);
    for (std::size_t v = 0; v < nVerts; v++) { vp[v] = toDouble(planet.voronoi_points[v]); }

    ringStart.assign(nCells + 1, 0);
    int total = 0;
    for (std::size_t i = 0; i < nCells; i++)
    {
        ringStart[i] = total;
        total += static_cast<int>(planet.cells[i].vertex.size());
    }
    ringStart[nCells] = total;

    ringEdge.assign(total, -1);
    ringSign.assign(total, 0);
    edges.clear();
    edges.reserve(total / 2 + 16);
    originalArea = 0.0;

    std::unordered_map<uint64_t, int> lookup;
    lookup.reserve(total);

    for (std::size_t i = 0; i < nCells; i++)
    {
        const Cell& cell = planet.cells[i];
        const std::size_t n = cell.vertex.size();
        const glm::dvec3 center = toDouble(planet.points[i]);

        for (std::size_t j = 0; j < n; j++)
        {
            int a = cell.vertex[j];
            int b = cell.vertex[(j + 1) % n];

            // Triangles on near zero length edges, or whose centre lies within the tolerance of the
            // edge's great circle, are treated as degenerate, float rounding can flip their sign
            // without anything moving
            const glm::dvec3 normal = glm::cross(vp[a], vp[b]);
            double c = glm::dot(center, normal);
            double edgeLength = glm::length(vp[b] - vp[a]);
            bool degenerate = edgeLength < degenerateTolerance || std::abs(c) < degenerateTolerance * glm::length(normal);
            ringSign[ringStart[i] + j] = degenerate ? 0 : (c > 0.0 ? 1 : -1);
            originalArea += sphericalTriangleArea(center, vp[a], vp[b]);

            if (a == b) { continue; }

            int lo = std::min(a, b), hi = std::max(a, b);
            uint64_t key = (static_cast<uint64_t>(lo) << 32) | static_cast<uint32_t>(hi);
            auto it = lookup.find(key);
            int index;
            if (it == lookup.end())
            {
                index = static_cast<int>(edges.size());
                Edge edge;
                edge.a = lo;
                edge.b = hi;
                edges.push_back(edge);
                lookup.emplace(key, index);
            }
            else { index = it->second; }

            Edge& edge = edges[index];
            if (edge.count < 2)
            {
                edge.cell[edge.count] = static_cast<int>(i);
                edge.forward[edge.count] = (a == edge.a);
            }
            edge.count++;
            ringEdge[ringStart[i] + j] = index;
        }
    }

    // The sphere is closed, so every edge is shared by two cells. One that isn't means a broken
    // ring, and its vertices are pinned so nothing moves next to it
    vertexOnOpenEdge.assign(nVerts, 0);
    for (Edge& edge : edges)
    {
        edge.shared = edge.count == 2 && edge.forward[0] != edge.forward[1];
        if (!edge.shared)
        {
            vertexOnOpenEdge[edge.a] = 1;
            vertexOnOpenEdge[edge.b] = 1;
        }
    }

    // Cells around every vertex (CSR), for rechecking only the cells a clamped vertex can change
    vertexCellStart.assign(nVerts + 1, 0);
    for (std::size_t i = 0; i < nCells; i++)
    {
        for (int v : planet.cells[i].vertex) { vertexCellStart[v + 1]++; }
    }
    for (std::size_t v = 0; v < nVerts; v++) { vertexCellStart[v + 1] += vertexCellStart[v]; }
    vertexCells.resize(vertexCellStart[nVerts]);
    std::vector<int> fill(vertexCellStart.begin(), vertexCellStart.end() - 1);
    for (std::size_t i = 0; i < nCells; i++)
    {
        for (int v : planet.cells[i].vertex) { vertexCells[fill[v]++] = static_cast<int>(i); }
    }

    topoCells = nCells;
    topoVoronoiPoints = nVerts;
    topologyValid = true;
}

void SphereBoundaries::buildGeometry(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode)
{
    auto start = std::chrono::high_resolution_clock::now();

    // Classify edges by the highest priority enabled type whose labels differ
    edgeType.assign(edges.size(), -1);
    unsigned int boundaryEdges = 0;
    for (std::size_t e = 0; e < edges.size(); e++)
    {
        const Edge& edge = edges[e];
        if (!edge.shared) { continue; }
        const Cell& c0 = planet.cells[edge.cell[0]];
        const Cell& c1 = planet.cells[edge.cell[1]];
        for (int t = 0; t < BORDER_TYPE_COUNT; t++)
        {
            if (settings.modeTypes[mapMode][t] && cellLabel(c0, t) != cellLabel(c1, t))
            {
                edgeType[e] = static_cast<signed char>(t);
                boundaryEdges++;
                break;
            }
        }
    }

    buildChains();

    // Strength per smoothed vertex, the fold-over guard lowers it locally
    const std::size_t nVerts = vp.size();
    vertexStrengthInitial.assign(nVerts, 0.f);
    for (const Chain& chain : chains)
    {
        float strength = settings.styles[static_cast<int>(chain.type)].strength;
        for (int v : chain.verts)
        {
            if (!pinned[v]) { vertexStrengthInitial[v] = strength; }
        }
    }
    vertexStrength = vertexStrengthInitial;

    // Sample layout, and the vertices, edges and cells that can change, so the clamp passes skip the rest
    sampleStart.assign(edges.size() + 1, 0);
    boundaryEdgeList.clear();
    for (std::size_t e = 0; e < edges.size(); e++)
    {
        int count = 0;
        if (edgeType[e] >= 0)
        {
            boundaryEdgeList.push_back(static_cast<int>(e));
            if (glm::length(vp[edges[e].b] - vp[edges[e].a]) >= subdivideTolerance)
            {
                count = std::max(settings.styles[edgeType[e]].subdivisions - 1, 0);
            }
        }
        sampleStart[e + 1] = sampleStart[e] + count;
    }
    samples.resize(sampleStart[edges.size()]);

    smoothedVertices.clear();
    for (std::size_t v = 0; v < nVerts; v++)
    {
        if (vertexNeighbors[2 * v] >= 0 && vertexStrengthInitial[v] > 0.f) { smoothedVertices.push_back(static_cast<int>(v)); }
    }

    cellAffected.assign(planet.cells.size(), 0);
    affectedCells.clear();
    for (std::size_t i = 0; i < planet.cells.size(); i++)
    {
        const Cell& cell = planet.cells[i];
        for (std::size_t j = 0; j < cell.vertex.size() && !cellAffected[i]; j++)
        {
            int e = ringEdge[ringStart[i] + j];
            if (vertexStrengthInitial[cell.vertex[j]] > 0.f || (e >= 0 && sampleStart[e + 1] > sampleStart[e])) { cellAffected[i] = 1; }
        }
        if (cellAffected[i]) { affectedCells.push_back(static_cast<int>(i)); }
    }
    positions = planet.voronoi_points;

    // Fold-over guard: halve the strength of the vertices that moved a flipped triangle, then only
    // recheck the cells around them. The last passes set the strength to 0, which restores the original fan
    std::vector<char> offenders(nVerts, 0);
    std::vector<char> queued(planet.cells.size(), 0);
    std::vector<int> checkCells = affectedCells;
    cellFlips.assign(planet.cells.size(), 0);
    flips = 0;
    for (int pass = 0; pass < MAX_CLAMP_PASSES; pass++)
    {
        computePositions();
        flips = collectFlips(planet, checkCells, offenders, flips);
        if (flips == 0 || pass == MAX_CLAMP_PASSES - 1) { break; }

        bool zero = pass >= MAX_CLAMP_PASSES - 3;
        checkCells.clear();
        for (std::size_t v = 0; v < nVerts; v++)
        {
            if (!offenders[v]) { continue; }
            offenders[v] = 0;
            vertexStrength[v] = zero ? 0.f : vertexStrength[v] * 0.5f;
            for (int k = vertexCellStart[v]; k < vertexCellStart[v + 1]; k++)
            {
                int cell = vertexCells[k];
                if (!queued[cell] && cellAffected[cell]) { queued[cell] = 1; checkCells.push_back(cell); }
            }
        }
        for (int cell : checkCells) { queued[cell] = 0; }
    }

    clampedVertices = 0;
    for (std::size_t v = 0; v < nVerts; v++)
    {
        if (vertexStrength[v] < vertexStrengthInitial[v] - 1e-6f) { clampedVertices++; }
    }

    buildRims(planet);
    buildPolylines();

    if (flips > 0)
    {
        LOG_WARNING(Graphics, "Sphere boundary: flipped triangles remain after clamping (count, first cell)", flips, firstFlipCell);
    }
    if (chainErrors > 0)
    {
        LOG_WARNING(Graphics, "Sphere boundary: inconsistent chain edges", chainErrors);
    }
    double relative = std::abs(rimArea - originalArea) / std::max(std::abs(originalArea), 1e-12);
    if (relative > 1e-4)
    {
        LOG_WARNING(Graphics, "Sphere boundary: render area differs from the cells' (relative difference)", relative);
    }

    geometryValid = true;

    auto end = std::chrono::high_resolution_clock::now();
    buildMs = std::chrono::duration<float, std::milli>(end - start).count();
    LOG_INFO(Graphics, "Sphere boundary geometry rebuilt (ms, mode, boundary edges, chains, clamped vertices)",
        buildMs, mapMode, boundaryEdges, static_cast<unsigned int>(chains.size()), clampedVertices);
}

void SphereBoundaries::buildChains()
{
    const std::size_t nVerts = vertexOnOpenEdge.size();
    chainErrors = 0;

    // Boundary edges per vertex (CSR)
    std::vector<int> degree(nVerts, 0);
    std::vector<unsigned char> typeMask(nVerts, 0);
    for (std::size_t e = 0; e < edges.size(); e++)
    {
        if (edgeType[e] < 0) { continue; }
        degree[edges[e].a]++;
        degree[edges[e].b]++;
        typeMask[edges[e].a] |= 1 << edgeType[e];
        typeMask[edges[e].b] |= 1 << edgeType[e];
    }
    std::vector<int> vStart(nVerts + 1, 0);
    for (std::size_t v = 0; v < nVerts; v++) { vStart[v + 1] = vStart[v] + degree[v]; }
    std::vector<int> vEdges(vStart[nVerts]);
    std::vector<int> fill(vStart.begin(), vStart.end() - 1);
    for (std::size_t e = 0; e < edges.size(); e++)
    {
        if (edgeType[e] < 0) { continue; }
        vEdges[fill[edges[e].a]++] = static_cast<int>(e);
        vEdges[fill[edges[e].b]++] = static_cast<int>(e);
    }

    // Pin chain ends: junctions of 3+ labels, where the boundary type changes, and broken rings
    pinned.assign(nVerts, 0);
    for (std::size_t v = 0; v < nVerts; v++)
    {
        if (degree[v] == 0) { continue; }
        bool mixedTypes = (typeMask[v] & (typeMask[v] - 1)) != 0;
        pinned[v] = vertexOnOpenEdge[v] || mixedTypes || degree[v] != 2;
    }

    auto other = [&](int e, int v) { return edges[e].a == v ? edges[e].b : edges[e].a; };
    auto nextEdge = [&](int v, int fromEdge)
    {
        for (int k = vStart[v]; k < vStart[v + 1]; k++)
        {
            if (vEdges[k] != fromEdge) { return vEdges[k]; }
        }
        return -1;
    };

    chains.clear();
    std::vector<char> visited(edges.size(), 0);

    // Open chains, walked from every pinned vertex
    for (std::size_t p = 0; p < nVerts; p++)
    {
        if (!pinned[p]) { continue; }
        for (int k = vStart[p]; k < vStart[p + 1]; k++)
        {
            int e = vEdges[k];
            if (visited[e]) { continue; }

            Chain chain;
            chain.type = static_cast<BorderType>(edgeType[e]);
            chain.verts.push_back(static_cast<int>(p));
            int cur = static_cast<int>(p);
            while (true)
            {
                visited[e] = 1;
                chain.edges.push_back(e);
                int next = other(e, cur);
                chain.verts.push_back(next);
                if (pinned[next]) { break; }
                int ne = nextEdge(next, e);
                if (ne < 0 || visited[ne]) { chainErrors++; break; }
                cur = next;
                e = ne;
            }
            chains.push_back(std::move(chain));
        }
    }

    // Closed loops, every vertex has exactly two edges of the same type
    for (std::size_t start = 0; start < edges.size(); start++)
    {
        if (edgeType[start] < 0 || visited[start]) { continue; }

        Chain chain;
        chain.type = static_cast<BorderType>(edgeType[start]);
        chain.closed = true;
        int first = edges[start].a;
        int cur = first;
        int e = static_cast<int>(start);
        chain.verts.push_back(first);
        while (true)
        {
            visited[e] = 1;
            chain.edges.push_back(e);
            int next = other(e, cur);
            if (next == first) { break; }
            chain.verts.push_back(next);
            int ne = nextEdge(next, e);
            if (ne < 0 || visited[ne]) { chainErrors++; chain.closed = false; break; }
            cur = next;
            e = ne;
        }
        chains.push_back(std::move(chain));
    }

    // Chain neighbours of every smoothed vertex
    vertexNeighbors.assign(nVerts * 2, -1);
    for (const Chain& chain : chains)
    {
        const std::size_t n = chain.verts.size();
        for (std::size_t i = 0; i < n; i++)
        {
            int v = chain.verts[i];
            if (pinned[v]) { continue; }
            if (!chain.closed && (i == 0 || i == n - 1)) { continue; }
            vertexNeighbors[2 * v] = chain.verts[(i + n - 1) % n];
            vertexNeighbors[2 * v + 1] = chain.verts[(i + 1) % n];
        }
    }
}

void SphereBoundaries::computePositions()
{
    // Corners move to their point on the curve: 1/4 m_prev + 1/2 v + 1/4 m_next, pushed back onto the sphere
    for (int v : smoothedVertices)
    {
        // Unmoved, the float position goes back exactly as it was
        if (vertexStrength[v] <= 0.f) { positions[v] = glm::vec3(vp[v]); continue; }
        int prev = vertexNeighbors[2 * v], next = vertexNeighbors[2 * v + 1];
        glm::dvec3 curve = glm::normalize(vp[prev] + vp[v] * 6.0 + vp[next]);
        positions[v] = glm::vec3(glm::normalize(glm::mix(vp[v], curve, static_cast<double>(vertexStrength[v]))));
    }

    // Interior samples. The half of an edge next to vertex v follows v's Bezier segment,
    // which runs from the midpoint of (prev, v) to the midpoint of (v, next) with v as control point
    auto otherNeighbor = [&](int v, int exclude)
    {
        return vertexNeighbors[2 * v] == exclude ? vertexNeighbors[2 * v + 1] : vertexNeighbors[2 * v];
    };

    for (int e : boundaryEdgeList)
    {
        const int count = sampleStart[e + 1] - sampleStart[e];
        if (count == 0) { continue; }

        const int a = edges[e].a, b = edges[e].b;
        const glm::dvec3& A = vp[a];
        const glm::dvec3& B = vp[b];
        const glm::dvec3 mid = (A + B) * 0.5;
        const int k = count + 1;
        const double sa = vertexNeighbors[2 * a] >= 0 ? vertexStrength[a] : 0.0;
        const double sb = vertexNeighbors[2 * b] >= 0 ? vertexStrength[b] : 0.0;

        for (int i = 0; i < count; i++)
        {
            double u = static_cast<double>(i + 1) / k;
            glm::dvec3 straight = glm::mix(A, B, u);
            glm::dvec3 point = straight;

            if (u <= 0.5 && sa > 0.0)
            {
                glm::dvec3 p0 = (vp[otherNeighbor(a, b)] + A) * 0.5;
                double t = 0.5 + u;
                glm::dvec3 curve = p0 * ((1 - t) * (1 - t)) + A * (2 * t * (1 - t)) + mid * (t * t);
                point = glm::mix(straight, curve, sa);
            }
            else if (u > 0.5 && sb > 0.0)
            {
                glm::dvec3 p2 = (B + vp[otherNeighbor(b, a)]) * 0.5;
                double t = u - 0.5;
                glm::dvec3 curve = mid * ((1 - t) * (1 - t)) + B * (2 * t * (1 - t)) + p2 * (t * t);
                point = glm::mix(straight, curve, sb);
            }
            samples[sampleStart[e] + i] = glm::vec3(glm::normalize(point));
        }
    }
}

void SphereBoundaries::buildRim(const vor::SphereVoronoi& planet, std::size_t cellIndex, std::vector<glm::vec3>& rim, std::vector<int>& rimOwner, std::vector<int>& rimRing) const
{
    // Ring of the cell with its smoothed corners and edge samples, the owner is the vertex whose strength moved the point
    rim.clear();
    rimOwner.clear();
    rimRing.clear();

    const Cell& cell = planet.cells[cellIndex];
    const std::size_t n = cell.vertex.size();
    for (std::size_t j = 0; j < n; j++)
    {
        int v = cell.vertex[j];
        rim.push_back(positions[v]);
        rimOwner.push_back(vertexStrength[v] > 0.f ? v : -1);
        rimRing.push_back(static_cast<int>(j));

        int e = ringEdge[ringStart[cellIndex] + j];
        if (e < 0) { continue; }
        const int count = sampleStart[e + 1] - sampleStart[e];
        if (count == 0) { continue; }

        const bool forward = edges[e].a == v;
        const int k = count + 1;
        for (int i = 0; i < count; i++)
        {
            int index = forward ? i : count - 1 - i;
            rim.push_back(samples[sampleStart[e] + index]);
            rimOwner.push_back((index + 1) * 2 <= k ? edges[e].a : edges[e].b);
            rimRing.push_back(static_cast<int>(j));
        }
    }
}

unsigned int SphereBoundaries::collectFlips(const vor::SphereVoronoi& planet, const std::vector<int>& cells, std::vector<char>& offenders, unsigned int total)
{
    // Rechecks the given cells, marks the vertices that moved their flipped triangles and returns the updated total
    std::vector<glm::vec3> rim;
    std::vector<int> rimOwner, rimRing;
    for (int i : cells)
    {
        buildRim(planet, i, rim, rimOwner, rimRing);

        unsigned int cellFlipCount = 0;
        const glm::dvec3 center = toDouble(planet.points[i]);
        const std::size_t m = rim.size();
        for (std::size_t r = 0; r < m; r++)
        {
            int sign = ringSign[ringStart[i] + rimRing[r]];
            std::size_t next = (r + 1) % m;
            const glm::dvec3 a = toDouble(rim[r]), b = toDouble(rim[next]);
            const double d = det(a, b, center);
            if (sign != 0 && d * sign > 0.0) { continue; }
            // Rings are counter clockwise from outside, so a degenerate edge's triangle is still
            // meant to be positive. It may start slightly negative from rounding, only a fold
            // beyond that counts. Without this, smoothing one end of a near zero length edge
            // folds it unnoticed
            if (sign == 0 && d > -degenerateTolerance * glm::length(b - a)) { continue; }

            cellFlipCount++;
            if (rimOwner[r] >= 0) { offenders[rimOwner[r]] = 1; }
            if (rimOwner[next] >= 0) { offenders[rimOwner[next]] = 1; }
        }

        total = total - cellFlips[i] + cellFlipCount;
        cellFlips[i] = cellFlipCount;
        if (cellFlipCount > 0) { firstFlipCell = i; }
    }
    return total;
}

void SphereBoundaries::buildRims(const vor::SphereVoronoi& planet)
{
    rims.clear();
    rims.reserve(planet.voronoi_points.size() * 2 + samples.size() * 2);
    rimStart.assign(planet.cells.size() + 1, 0);
    rimArea = 0.0;

    std::vector<glm::vec3> rim;
    std::vector<int> rimOwner, rimRing;
    for (std::size_t i = 0; i < planet.cells.size(); i++)
    {
        rimStart[i] = static_cast<int>(rims.size());
        buildRim(planet, i, rim, rimOwner, rimRing);
        rims.insert(rims.end(), rim.begin(), rim.end());

        const glm::dvec3 center = toDouble(planet.points[i]);
        for (std::size_t r = 0; r < rim.size(); r++)
        {
            rimArea += sphericalTriangleArea(center, toDouble(rim[r]), toDouble(rim[(r + 1) % rim.size()]));
        }
    }
    rimStart[planet.cells.size()] = static_cast<int>(rims.size());
}

void SphereBoundaries::buildPolylines()
{
    polylines.clear();
    polylines.reserve(chains.size());
    for (const Chain& chain : chains)
    {
        Polyline line;
        line.type = chain.type;
        line.closed = chain.closed;
        for (std::size_t i = 0; i < chain.edges.size(); i++)
        {
            int v = chain.verts[i];
            int e = chain.edges[i];
            line.points.push_back(positions[v]);

            const int count = sampleStart[e + 1] - sampleStart[e];
            const bool forward = edges[e].a == v;
            for (int s = 0; s < count; s++)
            {
                line.points.push_back(samples[sampleStart[e] + (forward ? s : count - 1 - s)]);
            }
        }
        if (!chain.closed) { line.points.push_back(positions[chain.verts.back()]); }
        polylines.push_back(std::move(line));
    }
}

void SphereBoundaries::appendOutlines(RibbonBatch& batch, const RenderSettings& settings) const
{
    if (!geometryValid) { return; }

    for (int t = BORDER_TYPE_COUNT - 1; t >= 0; t--)
    {
        const BoundaryStyle& style = settings.styles[t];
        if (!style.outline) { continue; }
        for (const Polyline& line : polylines)
        {
            if (static_cast<int>(line.type) == t) { batch.addLine(line.points, line.closed, style.outlineColor, style.outlineWidthPx); }
        }
    }

    std::unordered_map<int, std::pair<int, int>> ends; // vertex -> (count, highest priority type)
    for (const Chain& chain : chains)
    {
        if (chain.closed || !settings.styles[static_cast<int>(chain.type)].outline) { continue; }
        for (int v : { chain.verts.front(), chain.verts.back() })
        {
            auto it = ends.find(v);
            if (it == ends.end()) { ends.emplace(v, std::make_pair(1, static_cast<int>(chain.type))); }
            else
            {
                it->second.first++;
                it->second.second = std::min(it->second.second, static_cast<int>(chain.type));
            }
        }
    }
    for (const auto& [v, info] : ends)
    {
        if (info.first < 2) { continue; }
        const BoundaryStyle& style = settings.styles[info.second];
        batch.addDisc(positions[v], style.outlineColor, style.outlineWidthPx);
    }
}

std::vector<glm::vec3> SphereBoundaries::pinnedPoints() const
{
    std::vector<glm::vec3> points;
    for (std::size_t v = 0; v < pinned.size(); v++)
    {
        if (pinned[v]) { points.push_back(positions[v]); }
    }
    return points;
}

std::vector<glm::vec3> SphereBoundaries::clampedPoints() const
{
    std::vector<glm::vec3> points;
    for (std::size_t v = 0; v < vertexStrength.size(); v++)
    {
        if (vertexStrength[v] < vertexStrengthInitial[v] - 1e-6f) { points.push_back(positions[v]); }
    }
    return points;
}

std::vector<glm::vec3> SphereBoundaries::originalEdges(const vor::SphereVoronoi& planet) const
{
    std::vector<glm::vec3> points;
    for (std::size_t e = 0; e < edges.size(); e++)
    {
        if (edgeType[e] < 0) { continue; }
        points.push_back(planet.voronoi_points[edges[e].a]);
        points.push_back(planet.voronoi_points[edges[e].b]);
    }
    return points;
}
