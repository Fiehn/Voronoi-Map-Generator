#include "BoundaryRender.hpp"
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include "Logger.h"
#include "imgui.h"
#include "Include/nlohmann/json.hpp"

namespace {

    const char* BORDER_TYPE_NAMES[BORDER_TYPE_COUNT] = { "Coast", "Continent", "Biome" };
    const char* MAP_MODE_NAMES[MAP_MODE_COUNT] = { "Height", "Temperature", "Biome", "Percepitation", "Humidity", "Continents", "Cultures", "Resources" };

    constexpr int MAX_CLAMP_PASSES = 10; // Halving passes for the fold-over guard, the last two set the strength to 0
    constexpr float MITER_LIMIT = 2.f; // Max miter length in half widths

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

    // Twice the signed area of the triangle (a, b, c)
    double cross(const sf::Vector2f& a, const sf::Vector2f& b, const sf::Vector2f& c)
    {
        return (static_cast<double>(b.x) - a.x) * (static_cast<double>(c.y) - a.y)
            - (static_cast<double>(b.y) - a.y) * (static_cast<double>(c.x) - a.x);
    }

    sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, float t)
    {
        return a + (b - a) * t;
    }

    float length(const sf::Vector2f& v)
    {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    void addTriangle(sf::VertexArray& array, const sf::Vector2f& a, const sf::Vector2f& b, const sf::Vector2f& c, const sf::Color& color)
    {
        array.append(sf::Vertex(a, color));
        array.append(sf::Vertex(b, color));
        array.append(sf::Vertex(c, color));
    }

    void addDisc(sf::VertexArray& array, const sf::Vector2f& center, float radius, const sf::Color& color)
    {
        const int segments = 12;
        for (int i = 0; i < segments; i++)
        {
            float a0 = static_cast<float>(2.0 * PI * i / segments);
            float a1 = static_cast<float>(2.0 * PI * (i + 1) / segments);
            addTriangle(array, center,
                center + sf::Vector2f(std::cos(a0), std::sin(a0)) * radius,
                center + sf::Vector2f(std::cos(a1), std::sin(a1)) * radius, color);
        }
    }

    void addSquare(sf::VertexArray& array, const sf::Vector2f& center, float half, const sf::Color& color)
    {
        sf::Vector2f tl = center + sf::Vector2f(-half, -half), tr = center + sf::Vector2f(half, -half);
        sf::Vector2f bl = center + sf::Vector2f(-half, half), br = center + sf::Vector2f(half, half);
        addTriangle(array, tl, tr, br, color);
        addTriangle(array, tl, br, bl, color);
    }

    // Triangle ribbon along a polyline with clamped miter joins, butt ends on open lines
    void addRibbon(sf::VertexArray& array, const std::vector<sf::Vector2f>& input, bool closed, float halfWidth, const sf::Color& color)
    {
        std::vector<sf::Vector2f> p;
        p.reserve(input.size());
        for (const sf::Vector2f& point : input)
        {
            if (p.empty() || length(point - p.back()) > 1e-4f) { p.push_back(point); }
        }
        if (closed && p.size() > 1 && length(p.front() - p.back()) <= 1e-4f) { p.pop_back(); }

        const std::size_t n = p.size();
        if (n < 2) { return; }
        if (closed && n < 3) { closed = false; }
        const std::size_t segments = closed ? n : n - 1;

        std::vector<sf::Vector2f> normals(segments);
        for (std::size_t s = 0; s < segments; s++)
        {
            sf::Vector2f d = p[(s + 1) % n] - p[s];
            float len = length(d);
            normals[s] = sf::Vector2f(-d.y / len, d.x / len);
        }

        std::vector<sf::Vector2f> offsets(n);
        for (std::size_t i = 0; i < n; i++)
        {
            if (!closed && i == 0) { offsets[i] = normals[0] * halfWidth; continue; }
            if (!closed && i == n - 1) { offsets[i] = normals[segments - 1] * halfWidth; continue; }

            const sf::Vector2f& n1 = normals[(i + segments - 1) % segments];
            const sf::Vector2f& n2 = normals[i % segments];
            sf::Vector2f miter = n1 + n2;
            float len = length(miter);
            if (len < 1e-4f) { offsets[i] = n2 * halfWidth; continue; }
            miter /= len;
            float d = std::max(miter.x * n2.x + miter.y * n2.y, 1.f / MITER_LIMIT);
            offsets[i] = miter * (halfWidth / d);
        }

        for (std::size_t s = 0; s < segments; s++)
        {
            std::size_t i = s, j = (s + 1) % n;
            addTriangle(array, p[i] + offsets[i], p[i] - offsets[i], p[j] + offsets[j], color);
            addTriangle(array, p[i] - offsets[i], p[j] - offsets[j], p[j] + offsets[j], color);
        }
    }

    nlohmann::json colorToJson(const sf::Color& color)
    {
        return nlohmann::json::array({ color.r, color.g, color.b, color.a });
    }

    sf::Color colorFromJson(const nlohmann::json& j, const sf::Color& fallback)
    {
        if (!j.is_array() || j.size() != 4) { return fallback; }
        return sf::Color(j[0].get<sf::Uint8>(), j[1].get<sf::Uint8>(), j[2].get<sf::Uint8>(), j[3].get<sf::Uint8>());
    }
}

// ---------------------------------------------------------------------------------------------
// RenderSettings

RenderSettings::RenderSettings()
{
    BoundaryStyle& coast = styles[static_cast<int>(BorderType::Coast)];
    coast.subdivisions = 6;
    coast.strength = 1.f;
    coast.outlineWidthPx = 2.f;
    coast.outlineColor = sf::Color(25, 30, 40, 255);

    BoundaryStyle& continent = styles[static_cast<int>(BorderType::Continent)];
    continent.subdivisions = 6;
    continent.strength = 1.f;
    continent.outlineWidthPx = 2.5f;
    continent.outlineColor = sf::Color(170, 40, 40, 255);

    BoundaryStyle& biome = styles[static_cast<int>(BorderType::Biome)];
    biome.subdivisions = 4;
    biome.strength = 0.8f;
    biome.outlineWidthPx = 1.f;
    biome.outlineColor = sf::Color(30, 30, 30, 120);

    // Coast in every mode, plus the mode's own categorical boundary
    for (int mode = 0; mode < MAP_MODE_COUNT; mode++)
    {
        modeTypes[mode][static_cast<int>(BorderType::Coast)] = true;
    }
    modeTypes[2][static_cast<int>(BorderType::Biome)] = true;
    modeTypes[5][static_cast<int>(BorderType::Continent)] = true;
}

void RenderSettings::save_json(const std::string& filename) const
{
    nlohmann::json j;
    nlohmann::json jStyles = nlohmann::json::array();
    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        const BoundaryStyle& style = styles[t];
        jStyles.push_back({
            {"name", BORDER_TYPE_NAMES[t]},
            {"subdivisions", style.subdivisions},
            {"strength", style.strength},
            {"outline", style.outline},
            {"outline_width_px", style.outlineWidthPx},
            {"outline_color", colorToJson(style.outlineColor)}
            });
    }
    j["styles"] = jStyles;

    nlohmann::json jModes = nlohmann::json::array();
    for (int mode = 0; mode < MAP_MODE_COUNT; mode++)
    {
        nlohmann::json jMode = nlohmann::json::array();
        for (int t = 0; t < BORDER_TYPE_COUNT; t++) { jMode.push_back(modeTypes[mode][t]); }
        jModes.push_back(jMode);
    }
    j["mode_types"] = jModes;

    const HeightColors& height = colors.height;
    const TemperatureColors& temperature = colors.temperature;
    const PrecipitationColors& precipitation = colors.precipitation;
    j["map_colors"] = {
        {"height", {
            {"land", colorToJson(height.land)},
            {"ocean", colorToJson(height.ocean)},
            {"overflow", colorToJson(height.overflow)},
            {"alpha_floor", height.alphaFloor},
            {"steps_per_unit", height.stepsPerUnit}
        }},
        {"temperature", {
            {"cold", colorToJson(temperature.cold)},
            {"hot", colorToJson(temperature.hot)},
            {"cold_overflow", colorToJson(temperature.coldOverflow)},
            {"hot_overflow", colorToJson(temperature.hotOverflow)},
            {"steps_per_degree", temperature.stepsPerDegree}
        }},
        {"precipitation", {
            {"dry", colorToJson(precipitation.dry)},
            {"wet", colorToJson(precipitation.wet)},
            {"overflow", colorToJson(precipitation.overflow)},
            {"steps_per_unit", precipitation.stepsPerUnit}
        }}
    };

    std::ofstream o(filename);
    o << std::setw(4) << j << std::endl;
}

bool RenderSettings::load_json(const std::string& filename)
{
    std::ifstream i(filename);
    if (!i.is_open()) { return false; }

    nlohmann::json j;
    try { i >> j; }
    catch (const nlohmann::json::exception& e)
    {
        LOG_WARNING(Graphics, "Could not parse render settings", filename, std::string(e.what()));
        return false;
    }

    const nlohmann::json& jStyles = j.value("styles", nlohmann::json::array());
    for (int t = 0; t < BORDER_TYPE_COUNT && t < static_cast<int>(jStyles.size()); t++)
    {
        const nlohmann::json& js = jStyles[t];
        BoundaryStyle& style = styles[t];
        style.subdivisions = std::clamp(js.value("subdivisions", style.subdivisions), 1, 16);
        style.strength = std::clamp(js.value("strength", style.strength), 0.f, 1.f);
        style.outline = js.value("outline", style.outline);
        style.outlineWidthPx = std::clamp(js.value("outline_width_px", style.outlineWidthPx), 0.5f, 8.f);
        style.outlineColor = colorFromJson(js.value("outline_color", nlohmann::json()), style.outlineColor);
    }

    const nlohmann::json& jModes = j.value("mode_types", nlohmann::json::array());
    for (int mode = 0; mode < MAP_MODE_COUNT && mode < static_cast<int>(jModes.size()); mode++)
    {
        for (int t = 0; t < BORDER_TYPE_COUNT && t < static_cast<int>(jModes[mode].size()); t++)
        {
            if (jModes[mode][t].is_boolean()) { modeTypes[mode][t] = jModes[mode][t].get<bool>(); }
        }
    }

    const nlohmann::json& jColors = j.value("map_colors", nlohmann::json::object());

    const nlohmann::json& jHeight = jColors.value("height", nlohmann::json::object());
    HeightColors& height = colors.height;
    height.land = colorFromJson(jHeight.value("land", nlohmann::json()), height.land);
    height.ocean = colorFromJson(jHeight.value("ocean", nlohmann::json()), height.ocean);
    height.overflow = colorFromJson(jHeight.value("overflow", nlohmann::json()), height.overflow);
    height.alphaFloor = std::clamp(jHeight.value("alpha_floor", height.alphaFloor), 0, 255);
    height.stepsPerUnit = std::clamp(jHeight.value("steps_per_unit", height.stepsPerUnit), 1.f, 10000.f);

    const nlohmann::json& jTemperature = jColors.value("temperature", nlohmann::json::object());
    TemperatureColors& temperature = colors.temperature;
    temperature.cold = colorFromJson(jTemperature.value("cold", nlohmann::json()), temperature.cold);
    temperature.hot = colorFromJson(jTemperature.value("hot", nlohmann::json()), temperature.hot);
    temperature.coldOverflow = colorFromJson(jTemperature.value("cold_overflow", nlohmann::json()), temperature.coldOverflow);
    temperature.hotOverflow = colorFromJson(jTemperature.value("hot_overflow", nlohmann::json()), temperature.hotOverflow);
    temperature.stepsPerDegree = std::clamp(jTemperature.value("steps_per_degree", temperature.stepsPerDegree), 0.1f, 10000.f);

    const nlohmann::json& jPrecipitation = jColors.value("precipitation", nlohmann::json::object());
    PrecipitationColors& precipitation = colors.precipitation;
    precipitation.dry = colorFromJson(jPrecipitation.value("dry", nlohmann::json()), precipitation.dry);
    precipitation.wet = colorFromJson(jPrecipitation.value("wet", nlohmann::json()), precipitation.wet);
    precipitation.overflow = colorFromJson(jPrecipitation.value("overflow", nlohmann::json()), precipitation.overflow);
    precipitation.stepsPerUnit = std::clamp(jPrecipitation.value("steps_per_unit", precipitation.stepsPerUnit), 0.01f, 10000.f);

    geometryRevision++;
    styleRevision++;
    return true;
}

// ---------------------------------------------------------------------------------------------
// BoundaryRenderer

void BoundaryRenderer::invalidate()
{
    topologyValid = false;
    geometryValid = false;
    overlaysValid = false;
}

const std::vector<sf::Vertex>& BoundaryRenderer::sync(const vor::Voronoi& map, const RenderSettings& settings, int mapMode)
{
    lastMap = &map;
    mapMode = std::clamp(mapMode, 0, MAP_MODE_COUNT - 1);

    if (!topologyValid || topoCells != map.cells.size() || topoVoronoiPoints != map.voronoi_points.size())
    {
        buildTopology(map);
        geometryValid = false;
    }

    uint64_t key = computeKey(map, settings, mapMode);
    if (!geometryValid || key != geometryKey)
    {
        buildGeometry(map, settings, mapMode);
        geometryKey = key;
    }

    applyColors(map);
    return renderVertices;
}

uint64_t BoundaryRenderer::computeKey(const vor::Voronoi& map, const RenderSettings& settings, int mapMode) const
{
    // Labels can change without the map changing (new biomes), so they are part of the key.
    // The mode itself is not, modes with the same active types share the geometry
    uint64_t h = FNV_OFFSET;
    hashMix(h, settings.geometryRevision);
    hashMix(h, map.cells.size());
    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        if (!settings.modeTypes[mapMode][t]) { continue; }
        hashMix(h, static_cast<uint64_t>(t) + 1);
        for (const Cell& cell : map.cells)
        {
            hashMix(h, static_cast<uint64_t>(static_cast<int64_t>(cellLabel(cell, t))));
        }
    }
    return h;
}

void BoundaryRenderer::buildTopology(const vor::Voronoi& map)
{
    const std::size_t nCells = map.cells.size();
    const std::size_t nVerts = map.voronoi_points.size();

    ringStart.assign(nCells + 1, 0);
    int total = 0;
    for (std::size_t i = 0; i < nCells; i++)
    {
        ringStart[i] = total;
        total += static_cast<int>(map.cells[i].vertex.size());
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
        const Cell& cell = map.cells[i];
        const std::size_t n = cell.vertex.size();
        const sf::Vector2f& center = map.points[cell.id];

        for (std::size_t j = 0; j < n; j++)
        {
            int a = cell.vertex[j];
            int b = cell.vertex[(j + 1) % n];

            // Triangles on near zero length edges, or whose center lies within 0.01 px of the edge's line,
            // are treated as degenerate, float rounding can flip their sign without anything moving
            double c = cross(map.voronoi_points[a], map.voronoi_points[b], center);
            double edgeLength = length(map.voronoi_points[b] - map.voronoi_points[a]);
            bool degenerate = edgeLength < 0.01 || std::abs(c) / edgeLength < 0.01;
            ringSign[ringStart[i] + j] = degenerate ? 0 : (c > 0.0 ? 1 : -1);
            originalArea += 0.5 * c;

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

    // Edges that are not cleanly shared (map border, hull fan closing edges, broken rings) pin their vertices
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
        for (int v : map.cells[i].vertex) { vertexCellStart[v + 1]++; }
    }
    for (std::size_t v = 0; v < nVerts; v++) { vertexCellStart[v + 1] += vertexCellStart[v]; }
    vertexCells.resize(vertexCellStart[nVerts]);
    std::vector<int> fill(vertexCellStart.begin(), vertexCellStart.end() - 1);
    for (std::size_t i = 0; i < nCells; i++)
    {
        for (int v : map.cells[i].vertex) { vertexCells[fill[v]++] = static_cast<int>(i); }
    }

    topoCells = nCells;
    topoVoronoiPoints = nVerts;
    topologyValid = true;
}

void BoundaryRenderer::buildGeometry(const vor::Voronoi& map, const RenderSettings& settings, int mapMode)
{
    auto start = std::chrono::high_resolution_clock::now();

    // Classify edges by the highest priority enabled type whose labels differ
    edgeType.assign(edges.size(), -1);
    unsigned int boundaryEdges = 0;
    for (std::size_t e = 0; e < edges.size(); e++)
    {
        const Edge& edge = edges[e];
        if (!edge.shared) { continue; }
        const Cell& c0 = map.cells[edge.cell[0]];
        const Cell& c1 = map.cells[edge.cell[1]];
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
    const std::size_t nVerts = map.voronoi_points.size();
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
            // Nearly coincident Voronoi vertices give sub-pixel edges, samples on them are below float precision
            float edgeLength = length(map.voronoi_points[edges[e].b] - map.voronoi_points[edges[e].a]);
            if (edgeLength >= 0.05f) { count = std::max(settings.styles[edgeType[e]].subdivisions - 1, 0); }
        }
        sampleStart[e + 1] = sampleStart[e] + count;
    }
    samples.resize(sampleStart[edges.size()]);

    smoothedVertices.clear();
    for (std::size_t v = 0; v < nVerts; v++)
    {
        if (vertexNeighbors[2 * v] >= 0 && vertexStrengthInitial[v] > 0.f) { smoothedVertices.push_back(static_cast<int>(v)); }
    }

    cellAffected.assign(map.cells.size(), 0);
    affectedCells.clear();
    for (std::size_t i = 0; i < map.cells.size(); i++)
    {
        const Cell& cell = map.cells[i];
        for (std::size_t j = 0; j < cell.vertex.size() && !cellAffected[i]; j++)
        {
            int e = ringEdge[ringStart[i] + j];
            if (vertexStrengthInitial[cell.vertex[j]] > 0.f || (e >= 0 && sampleStart[e + 1] > sampleStart[e])) { cellAffected[i] = 1; }
        }
        if (cellAffected[i]) { affectedCells.push_back(static_cast<int>(i)); }
    }
    positions = map.voronoi_points;

    // Fold-over guard: halve the strength of the vertices that moved a flipped triangle, then only
    // recheck the cells around them. The last passes set the strength to 0, which restores the original fan
    std::vector<char> offenders(nVerts, 0);
    std::vector<char> queued(map.cells.size(), 0);
    std::vector<int> checkCells = affectedCells;
    cellFlips.assign(map.cells.size(), 0);
    unsigned int flips = 0;
    for (int pass = 0; pass < MAX_CLAMP_PASSES; pass++)
    {
        computePositions(map);
        flips = collectFlips(map, checkCells, offenders, flips);
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

    buildRenderMesh(map);
    buildPolylines(settings);

#ifdef _DEBUG
    if (flips > 0)
    {
        LOG_WARNING(Graphics, "Boundary: flipped triangles remain after clamping (count, first cell)", flips, firstFlipCell);
    }
    if (chainErrors > 0)
    {
        LOG_WARNING(Graphics, "Boundary: inconsistent chain edges", chainErrors);
    }
    double area = 0.0;
    for (std::size_t i = 0; i + 2 < renderVertices.size(); i += 3)
    {
        area += 0.5 * cross(renderVertices[i].position, renderVertices[i + 1].position, renderVertices[i + 2].position);
    }
    double relative = std::abs(area - originalArea) / std::max(std::abs(originalArea), 1.0);
    if (relative > 1e-4)
    {
        LOG_WARNING(Graphics, "Boundary: render mesh area differs from the original (relative difference)", relative);
    }
#endif

    geometryValid = true;
    geometryStamp++;

    auto end = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(end - start).count();
    LOG_INFO(Graphics, "Boundary geometry rebuilt (ms, mode, boundary edges, chains, clamped vertices)",
        ms, mapMode, boundaryEdges, static_cast<unsigned int>(chains.size()), clampedVertices);
}

void BoundaryRenderer::buildChains()
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

    // Pin chain ends: map border, junctions of 3+ labels, and where the boundary type changes
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

void BoundaryRenderer::computePositions(const vor::Voronoi& map)
{
    const std::vector<sf::Vector2f>& vp = map.voronoi_points;

    // Corners move to their point on the curve: 1/4 m_prev + 1/2 v + 1/4 m_next
    for (int v : smoothedVertices)
    {
        int prev = vertexNeighbors[2 * v], next = vertexNeighbors[2 * v + 1];
        sf::Vector2f curve = (vp[prev] + vp[v] * 6.f + vp[next]) / 8.f;
        positions[v] = lerp(vp[v], curve, vertexStrength[v]);
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
        const sf::Vector2f& A = vp[a];
        const sf::Vector2f& B = vp[b];
        const sf::Vector2f mid = (A + B) * 0.5f;
        const int k = count + 1;
        const float sa = vertexNeighbors[2 * a] >= 0 ? vertexStrength[a] : 0.f;
        const float sb = vertexNeighbors[2 * b] >= 0 ? vertexStrength[b] : 0.f;

        for (int i = 0; i < count; i++)
        {
            float u = static_cast<float>(i + 1) / k;
            sf::Vector2f straight = lerp(A, B, u);
            sf::Vector2f point = straight;

            if (u <= 0.5f && sa > 0.f)
            {
                sf::Vector2f p0 = (vp[otherNeighbor(a, b)] + A) * 0.5f;
                float t = 0.5f + u;
                sf::Vector2f curve = p0 * ((1 - t) * (1 - t)) + A * (2 * t * (1 - t)) + mid * (t * t);
                point = lerp(straight, curve, sa);
            }
            else if (u > 0.5f && sb > 0.f)
            {
                sf::Vector2f p2 = (B + vp[otherNeighbor(b, a)]) * 0.5f;
                float t = u - 0.5f;
                sf::Vector2f curve = mid * ((1 - t) * (1 - t)) + B * (2 * t * (1 - t)) + p2 * (t * t);
                point = lerp(straight, curve, sb);
            }
            samples[sampleStart[e] + i] = point;
        }
    }
}

void BoundaryRenderer::buildRim(const vor::Voronoi& map, std::size_t cellIndex, std::vector<sf::Vector2f>& rim, std::vector<int>& rimOwner, std::vector<int>& rimRing) const
{
    // Ring of the cell with its smoothed corners and edge samples, the owner is the vertex whose strength moved the point
    rim.clear();
    rimOwner.clear();
    rimRing.clear();

    const Cell& cell = map.cells[cellIndex];
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

unsigned int BoundaryRenderer::collectFlips(const vor::Voronoi& map, const std::vector<int>& cells, std::vector<char>& offenders, unsigned int total)
{
    // Rechecks the given cells, marks the vertices that moved their flipped triangles and returns the updated total
    std::vector<sf::Vector2f> rim;
    std::vector<int> rimOwner, rimRing;
    for (int i : cells)
    {
        buildRim(map, i, rim, rimOwner, rimRing);

        unsigned int flips = 0;
        const sf::Vector2f& center = map.points[map.cells[i].id];
        const std::size_t m = rim.size();
        for (std::size_t r = 0; r < m; r++)
        {
            int sign = ringSign[ringStart[i] + rimRing[r]];
            if (sign == 0) { continue; }
            std::size_t next = (r + 1) % m;
            if (cross(rim[r], rim[next], center) * sign > 0.0) { continue; }

            flips++;
            if (rimOwner[r] >= 0) { offenders[rimOwner[r]] = 1; }
            if (rimOwner[next] >= 0) { offenders[rimOwner[next]] = 1; }
        }

        total = total - cellFlips[i] + flips;
        cellFlips[i] = flips;
        if (flips > 0) { firstFlipCell = i; }
    }
    return total;
}

void BoundaryRenderer::buildRenderMesh(const vor::Voronoi& map)
{
    // Same fan layout as Voronoi::vertexGen (v_j, v_j+1, center), with the extra rim points included
    renderStart.assign(map.cells.size(), 0);
    renderCount.assign(map.cells.size(), 0);
    renderVertices.clear();
    renderVertices.reserve(map.vertices.size() + samples.size() * 6);

    std::vector<sf::Vector2f> rim;
    std::vector<int> rimOwner, rimRing;
    for (std::size_t i = 0; i < map.cells.size(); i++)
    {
        renderStart[i] = static_cast<int>(renderVertices.size());
        if (map.cells[i].vertex.empty()) { continue; }

        // Untouched cells are identical to their fan in map.vertices
        if (!cellAffected[i])
        {
            auto first = map.vertices.begin() + map.cells[i].vertex_offset;
            renderVertices.insert(renderVertices.end(), first, first + map.cells[i].vertex.size() * 3);
            renderCount[i] = static_cast<int>(map.cells[i].vertex.size() * 3);
            continue;
        }
        buildRim(map, i, rim, rimOwner, rimRing);

        const sf::Vector2f& center = map.points[map.cells[i].id];
        const std::size_t m = rim.size();
        for (std::size_t r = 0; r < m; r++)
        {
            renderVertices.push_back(sf::Vertex(rim[r], sf::Color::White));
            renderVertices.push_back(sf::Vertex(rim[(r + 1) % m], sf::Color::White));
            renderVertices.push_back(sf::Vertex(center, sf::Color::White));
        }
        renderCount[i] = static_cast<int>(renderVertices.size()) - renderStart[i];
    }
}

void BoundaryRenderer::buildPolylines(const RenderSettings& settings)
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

void BoundaryRenderer::applyColors(const vor::Voronoi& map)
{
    for (std::size_t i = 0; i < map.cells.size() && i < renderStart.size(); i++)
    {
        if (renderCount[i] == 0) { continue; }
        const sf::Color color = map.vertices[map.cells[i].vertex_offset].color;
        for (int k = renderStart[i]; k < renderStart[i] + renderCount[i]; k++)
        {
            renderVertices[k].color = color;
        }
    }
}

void BoundaryRenderer::buildOverlays(const vor::Voronoi& map, const RenderSettings& settings, float worldPerPixel)
{
    outlines.clear();
    debugLines.clear();
    debugDots.clear();

    // Lowest priority first so the coast is drawn on top
    for (int t = BORDER_TYPE_COUNT - 1; t >= 0; t--)
    {
        const BoundaryStyle& style = settings.styles[t];
        if (!style.outline) { continue; }
        const float halfWidth = 0.5f * style.outlineWidthPx * worldPerPixel;
        for (const Polyline& line : polylines)
        {
            if (static_cast<int>(line.type) == t) { addRibbon(outlines, line.points, line.closed, halfWidth, style.outlineColor); }
        }
    }

    // Discs where several outlined chains end in the same vertex, so junctions don't show notches
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
        addDisc(outlines, positions[v], 0.5f * style.outlineWidthPx * worldPerPixel, style.outlineColor);
    }

    if (settings.debugWireframe)
    {
        const sf::Color color(0, 0, 0, 90);
        for (std::size_t i = 0; i + 2 < renderVertices.size(); i += 3)
        {
            for (int k = 0; k < 3; k++)
            {
                debugLines.append(sf::Vertex(renderVertices[i + k].position, color));
                debugLines.append(sf::Vertex(renderVertices[i + (k + 1) % 3].position, color));
            }
        }
    }
    if (settings.debugOriginalEdges)
    {
        const sf::Color color(255, 0, 255, 200);
        for (std::size_t e = 0; e < edges.size(); e++)
        {
            if (edgeType[e] < 0) { continue; }
            debugLines.append(sf::Vertex(map.voronoi_points[edges[e].a], color));
            debugLines.append(sf::Vertex(map.voronoi_points[edges[e].b], color));
        }
    }
    if (settings.debugChains)
    {
        for (std::size_t c = 0; c < polylines.size(); c++)
        {
            uint64_t h = FNV_OFFSET;
            hashMix(h, c);
            const sf::Color color(
                static_cast<sf::Uint8>(64 + (h & 0xBF)),
                static_cast<sf::Uint8>(64 + ((h >> 8) & 0xBF)),
                static_cast<sf::Uint8>(64 + ((h >> 16) & 0xBF)), 255);
            const std::vector<sf::Vector2f>& p = polylines[c].points;
            for (std::size_t i = 0; i + 1 < p.size(); i++)
            {
                debugLines.append(sf::Vertex(p[i], color));
                debugLines.append(sf::Vertex(p[i + 1], color));
            }
            if (polylines[c].closed && p.size() > 2)
            {
                debugLines.append(sf::Vertex(p.back(), color));
                debugLines.append(sf::Vertex(p.front(), color));
            }
        }
        for (std::size_t v = 0; v < pinned.size(); v++)
        {
            if (pinned[v]) { addSquare(debugDots, positions[v], 2.5f * worldPerPixel, sf::Color::Red); }
        }
    }
    if (settings.debugClamped)
    {
        for (std::size_t v = 0; v < vertexStrength.size(); v++)
        {
            if (vertexStrength[v] < vertexStrengthInitial[v] - 1e-6f) { addSquare(debugDots, positions[v], 3.5f * worldPerPixel, sf::Color::Yellow); }
        }
    }
}

void BoundaryRenderer::draw(sf::RenderWindow& window, const RenderSettings& settings)
{
    if (!geometryValid || lastMap == nullptr) { return; }

    const float worldPerPixel = window.getView().getSize().x / static_cast<float>(window.getSize().x);
    if (!overlaysValid || overlayGeometryStamp != geometryStamp || overlayStyleRevision != settings.styleRevision
        || std::abs(worldPerPixel - overlayWorldPerPixel) > 1e-4f * overlayWorldPerPixel)
    {
        buildOverlays(*lastMap, settings, worldPerPixel);
        overlaysValid = true;
        overlayGeometryStamp = geometryStamp;
        overlayStyleRevision = settings.styleRevision;
        overlayWorldPerPixel = worldPerPixel;
    }

    window.draw(outlines);
    if (debugLines.getVertexCount() > 0) { window.draw(debugLines); }
    if (debugDots.getVertexCount() > 0) { window.draw(debugDots); }
}

// ---------------------------------------------------------------------------------------------
// UI

bool boundarySettingsUI(RenderSettings& settings, int mapMode)
{
    if (!ImGui::CollapsingHeader("Boundaries")) { return false; }

    bool geometryChanged = false;
    mapMode = std::clamp(mapMode, 0, MAP_MODE_COUNT - 1);

    // Own ID scope, the tree nodes below use the same labels
    ImGui::Text("Smoothed in %s mode:", MAP_MODE_NAMES[mapMode]);
    ImGui::PushID("ModeTypes");
    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        if (t > 0) { ImGui::SameLine(); }
        if (ImGui::Checkbox(BORDER_TYPE_NAMES[t], &settings.modeTypes[mapMode][t])) { geometryChanged = true; }
    }
    ImGui::PopID();

    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        ImGui::PushID(t);
        if (ImGui::TreeNode(BORDER_TYPE_NAMES[t]))
        {
            BoundaryStyle& style = settings.styles[t];

            // Geometry rebuilds when the slider is released
            ImGui::SliderInt("Subdivisions", &style.subdivisions, 1, 16);
            if (ImGui::IsItemDeactivatedAfterEdit()) { geometryChanged = true; }
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Segments per boundary edge"); ImGui::EndTooltip(); }

            ImGui::SliderFloat("Strength", &style.strength, 0.f, 1.f, "%.2f");
            if (ImGui::IsItemDeactivatedAfterEdit()) { geometryChanged = true; }
            if (ImGui::BeginItemTooltip()) { ImGui::Text("0 is the original Voronoi edges, 1 is the full curve"); ImGui::EndTooltip(); }

            // Outlines are cheap to rebuild, so they update live
            if (ImGui::Checkbox("Outline", &style.outline)) { settings.styleRevision++; }
            if (ImGui::SliderFloat("Width (px)", &style.outlineWidthPx, 0.5f, 8.f, "%.1f")) { settings.styleRevision++; }
            float color[4] = { style.outlineColor.r / 255.f, style.outlineColor.g / 255.f, style.outlineColor.b / 255.f, style.outlineColor.a / 255.f };
            if (ImGui::ColorEdit4("Color", color))
            {
                style.outlineColor = sf::Color(
                    static_cast<sf::Uint8>(color[0] * 255.f), static_cast<sf::Uint8>(color[1] * 255.f),
                    static_cast<sf::Uint8>(color[2] * 255.f), static_cast<sf::Uint8>(color[3] * 255.f));
                settings.styleRevision++;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (ImGui::TreeNode("Debug"))
    {
        if (ImGui::Checkbox("Wireframe", &settings.debugWireframe)) { settings.styleRevision++; }
        if (ImGui::Checkbox("Chains and pinned vertices", &settings.debugChains)) { settings.styleRevision++; }
        if (ImGui::Checkbox("Clamped vertices", &settings.debugClamped)) { settings.styleRevision++; }
        if (ImGui::Checkbox("Original edges", &settings.debugOriginalEdges)) { settings.styleRevision++; }
        ImGui::TreePop();
    }

    if (geometryChanged) { settings.geometryRevision++; }
    return geometryChanged;
}

bool renderSettingsFileUI(RenderSettings& settings)
{
    if (ImGui::Button("Save Style")) { settings.save_json(RENDER_SETTINGS_FILE); }
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Saves the boundaries and map colours"); ImGui::EndTooltip(); }
    ImGui::SameLine();
    if (ImGui::Button("Load Style"))
    {
        if (settings.load_json(RENDER_SETTINGS_FILE)) { return true; }
        LOG_WARNING(Graphics, "Could not load render settings", std::string(RENDER_SETTINGS_FILE));
    }
    return false;
}
