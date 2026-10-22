#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <SFML/Graphics.hpp>
#include "Voronoi.hpp"
#include "MapColors.hpp"

// Smooth boundary rendering
// Builds a render mesh from the Voronoi geometry where the edges between differently labelled cells
// are subdivided and smoothed (quadratic Bezier corner-cutting through the edge midpoints)

enum class BorderType : int
{
    // Ordered by priority, an edge gets the first enabled type whose labels differ across it
    Coast = 0,  // oceanBool differs
    Continent,  // continent differs
    // TODO: Culture boundaries (cell.culture) once the Cultures map is implemented.
    Biome,      // biome differs
    Count
};

constexpr int BORDER_TYPE_COUNT = static_cast<int>(BorderType::Count);
constexpr int MAP_MODE_COUNT = 8; // Matches the mapType radio buttons in main.cpp
inline constexpr const char* RENDER_SETTINGS_FILE = "render_settings.json";

struct BoundaryStyle
{
    int subdivisions = 6; // Segments per boundary edge, 1 only moves the corners
    float strength = 1.f; // 0 is the original Voronoi edges and 1 is the full curve
    bool outline = true;
    float outlineWidthPx = 2.f; // Screen pixels, the outline keeps its width when zooming
    sf::Color outlineColor = sf::Color(20, 20, 20, 255);
};

struct RenderSettings
{
    BoundaryStyle styles[BORDER_TYPE_COUNT];
    bool modeTypes[MAP_MODE_COUNT][BORDER_TYPE_COUNT] = {}; // Active boundary types for each map mode
    MapColors colors; // Height, temperature and precipitation colours

    // Debug overlays (not saved)
    bool debugWireframe = false;
    bool debugChains = false;
    bool debugClamped = false;
    bool debugOriginalEdges = false;

    // Bumped on change so the renderer knows what to rebuild (not saved)
    unsigned int geometryRevision = 0;
    unsigned int styleRevision = 0;

    RenderSettings();

    void save_json(const std::string& filename) const;
    bool load_json(const std::string& filename); // Missing keys keep their defaults, returns false if the file can't be read
};

class BoundaryRenderer
{
public:
    // Call when the Voronoi geometry has been regenerated
    void invalidate();

    // Rebuilds the smoothed geometry if the map, the labels or the settings changed,
    // then copies the cell colours from map.vertices onto the render mesh
    const std::vector<sf::Vertex>& sync(const vor::Voronoi& map, const RenderSettings& settings, int mapMode);

    // Draws outlines and debug overlays, the ribbons are rebuilt when the zoom or the style changes
    void draw(sf::RenderWindow& window, const RenderSettings& settings);

private:
    struct Edge
    {
        int a, b; // Voronoi vertex ids (a < b)
        int cell[2] = { -1, -1 };
        bool forward[2] = { false, false }; // The cell ring walks the edge from a to b
        int count = 0; // Number of cell rings using this edge
        bool shared = false; // Used by exactly two cells that walk it in opposite directions
    };

    struct Chain
    {
        BorderType type;
        bool closed = false;
        std::vector<int> verts; // Closed chains do not repeat the first vertex
        std::vector<int> edges; // edges[i] connects verts[i] and verts[i + 1]
    };

    struct Polyline
    {
        BorderType type;
        bool closed;
        std::vector<sf::Vector2f> points;
    };

    // Topology, only depends on the Voronoi geometry
    bool topologyValid = false;
    std::size_t topoCells = 0, topoVoronoiPoints = 0;
    std::vector<Edge> edges;
    std::vector<int> ringStart; // Offset of each cell's ring into ringEdge/ringSign (size cells + 1)
    std::vector<int> ringEdge; // Edge index for ring position j (vertex[j] -> vertex[j + 1])
    std::vector<signed char> ringSign; // Winding sign of the original fan triangle
    std::vector<char> vertexOnOpenEdge; // Vertex touches an edge that is not shared by exactly two cells
    std::vector<int> vertexCellStart, vertexCells; // Cells around each vertex (CSR)
    double originalArea = 0.0;

    // Geometry, depends on labels and settings
    bool geometryValid = false;
    uint64_t geometryKey = 0;
    std::vector<signed char> edgeType; // -1 for no boundary
    std::vector<char> pinned;
    std::vector<int> vertexNeighbors; // Two chain neighbours per smoothed vertex (-1 when unused)
    std::vector<float> vertexStrength;
    std::vector<float> vertexStrengthInitial;
    std::vector<sf::Vector2f> positions; // Smoothed vertex positions
    std::vector<int> sampleStart; // Per edge offset into samples (size edges + 1)
    std::vector<sf::Vector2f> samples; // Interior points of boundary edges, ordered from a to b
    std::vector<int> boundaryEdgeList; // Edges with a boundary type
    std::vector<int> smoothedVertices; // Vertices that move along a curve
    std::vector<int> affectedCells; // Cells with a moved vertex or a subdivided edge
    std::vector<char> cellAffected;
    std::vector<unsigned int> cellFlips; // Flipped triangles per cell from the last check
    std::vector<Chain> chains;
    std::vector<Polyline> polylines;
    std::vector<int> renderStart, renderCount; // Per cell range in renderVertices
    std::vector<sf::Vertex> renderVertices;
    unsigned int clampedVertices = 0;
    unsigned int chainErrors = 0;
    int firstFlipCell = -1; // For the debug log

    // Overlays, depend on geometry, style and zoom
    bool overlaysValid = false;
    unsigned int overlayGeometryStamp = 0, geometryStamp = 0;
    unsigned int overlayStyleRevision = 0;
    float overlayWorldPerPixel = 0.f;
    sf::VertexArray outlines = sf::VertexArray(sf::Triangles);
    sf::VertexArray debugLines = sf::VertexArray(sf::Lines);
    sf::VertexArray debugDots = sf::VertexArray(sf::Triangles);

    void buildTopology(const vor::Voronoi& map);
    void buildGeometry(const vor::Voronoi& map, const RenderSettings& settings, int mapMode);
    void buildChains();
    void computePositions(const vor::Voronoi& map);
    unsigned int collectFlips(const vor::Voronoi& map, const std::vector<int>& cells, std::vector<char>& offenders, unsigned int total);
    void buildRim(const vor::Voronoi& map, std::size_t cell, std::vector<sf::Vector2f>& rim, std::vector<int>& rimOwner, std::vector<int>& rimRing) const;
    void buildRenderMesh(const vor::Voronoi& map);
    void buildPolylines(const RenderSettings& settings);
    void buildOverlays(const vor::Voronoi& map, const RenderSettings& settings, float worldPerPixel);
    void applyColors(const vor::Voronoi& map);
    uint64_t computeKey(const vor::Voronoi& map, const RenderSettings& settings, int mapMode) const;

    const vor::Voronoi* lastMap = nullptr;
};

// Boundaries section for the Map Controls window, returns true when the geometry has to be rebuilt
bool boundarySettingsUI(RenderSettings& settings, int mapMode);

// Save and load buttons for the whole render settings file, returns true when the settings were loaded
bool renderSettingsFileUI(RenderSettings& settings);
