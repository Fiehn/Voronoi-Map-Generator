#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/Color.hpp>
#include "SphereVoronoi.hpp"
#include "PlanetCamera.hpp"
#include "Ribbon.hpp"
#include "ArrowField.hpp"
#include "RenderSettings.hpp"

// CPU side of the planet's render mesh, shared by the globe and the flat view. Kept apart from
// the GL upload so it can be checked without a window.
//
// Every cell is a fan from its centre around its ring, with its own copy of the ring vertices
// so it can be coloured flat. Each vertex has two positions: on the unit sphere for the globe,
// and lat/lon for the flat map. The longitude is unwrapped around the vertex's own cell centre,
// so a cell on the 180th meridian keeps its ring together (179.5 and 180.4 rather than 179.5
// and -179.6) and the flat view never draws a triangle across the whole map.
//
// That fails for the (up to) two cells holding a pole, whose rings go all the way round in
// longitude. The flat view draws those as bands instead: the ring unwrapped over a full turn,
// with a quad from each ring edge up to the pole latitude.
//
// Index order is [pole cell fans][other fans][pole bands], so the globe draws
// [0, globeIndexCount) and the flat view draws [poleFanIndexCount, indices.size()).
struct PlanetMesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> latLon;     // Radians
    std::vector<unsigned int> indices;
    int poleFanIndexCount = 0;
    int globeIndexCount = 0;

    // Fan vertices of cell i are [cellFirstVertex[i], cellFirstVertex[i + 1])
    std::vector<int> cellFirstVertex;
    struct Band { int cell; int firstVertex; int vertexCount; };
    std::vector<Band> poleBands;      // Vertices come after all the fans
    float lonMin = 0.f, lonMax = 0.f; // Radians, of the vertices the flat view draws
};

// The cells as they are, or with their rims (smoothed rings) from SphereBoundaries: the rim of
// cell i is rims[rimStart[i], rimStart[i + 1]), counter clockwise like its ring
PlanetMesh buildPlanetMesh(const vor::SphereVoronoi& sphere);
PlanetMesh buildPlanetMesh(const vor::SphereVoronoi& sphere, const std::vector<glm::vec3>& rims, const std::vector<int>& rimStart);

class River;

// Outline of a cell's original polygon (not the smoothed rim), as a closed ribbon line
void appendCellOutline(RibbonBatch& batch, const vor::SphereVoronoi& sphere, int cell, const OverlayStyle& style);

// Each river as the centres of its path cells, in River::getPathCells order. That walk goes back
// along a branch to reach the next one, as the flat map drew it
std::vector<std::vector<glm::vec3>> riverPaths(const vor::SphereVoronoi& sphere, const std::vector<River>& rivers);

// The circle of latitude lat (degrees), sampled every degree of longitude
void appendLatitudeCircle(RibbonBatch& batch, float lat, const OverlayStyle& style);

// Latitude in degrees of a convergence line, which is stored as a fraction of the map height
// from the north (the inverse of latToMapFraction in PlanetGen)
inline float convergenceLineLat(float fraction) { return 90.f - 180.f * fraction; }

// Ribbon layers, drawn in this order over the cells. The debug overlays go in before FoundCells
enum class RibbonGroup { Rivers, Boundaries, ConvergenceLines, FoundCells, Highlight, Count };
constexpr int RIBBON_GROUP_COUNT = static_cast<int>(RibbonGroup::Count);

// Arrow layers, drawn in this order after ConvergenceLines
enum class ArrowGroup { Plates, Wind, Count };
constexpr int ARROW_GROUP_COUNT = static_cast<int>(ArrowGroup::Count);

// Debug overlay vertices, drawn as lines (pairs) or round points of the given pixel size
struct DebugVertex {
    glm::vec3 position;
    glm::vec2 latLon; // Radians
    sf::Color color;
    float size = 1.f; // Points only, pixels
};
// Each pair of points is a segment, its second longitude unwrapped next to the first
void appendDebugLines(std::vector<DebugVertex>& out, const std::vector<glm::vec3>& pairs, sf::Color color);
void appendDebugPoints(std::vector<DebugVertex>& out, const std::vector<glm::vec3>& points, sf::Color color, float size);

// Copies of the flat map to draw so a longitude range [lonMin, lonMax] (radians) covers the
// window: copy k is shifted by k world widths. A static map only draws the copies that reach
// into 180W..180E, where it is clipped. False if no copy is in view
struct FlatCopies {
    int first = 0, last = -1;
    float maxX = 0.f; // Clip planes at |x| = maxX: the map edges, or far outside every copy when wrapping
    int count() const { return last - first + 1; }
};
bool flatCopies(const PlanetCamera& camera, const glm::vec2& viewport, float lonMin, float lonMax, FlatCopies& out);

// Draws the planet's cells with raw OpenGL 3.3, on the globe or as a flat map depending on the
// camera's mode, with ribbon lines and debug overlays over them. Positions are static and colours
// are a separate buffer, so recolouring the map is one upload.
//
// Both programs share the projection code. The cells use the depth test on the globe; ribbons
// don't, they lie on the surface and are clipped at the horizon instead, so they never z-fight
// or poke through near the limb.
//
// Every call needs the window's GL context to be active. draw() leaves no VAO, program or
// buffer bound and depth testing and clipping off, so SFML and ImGui can draw after it once
// window.resetGLStates() has run.
class GlobeRenderer {
public:
    GlobeRenderer() = default;
    ~GlobeRenderer();
    GlobeRenderer(const GlobeRenderer&) = delete;
    GlobeRenderer& operator=(const GlobeRenderer&) = delete;

    // The cells come out grey, without ribbons or debug overlays, until those are set again
    void build(const vor::SphereVoronoi& sphere);
    void build(const vor::SphereVoronoi& sphere, const std::vector<glm::vec3>& rims, const std::vector<int>& rimStart);
    void setCellColors(const std::vector<sf::Color>& colors); // One per cell
    void setRibbons(RibbonGroup group, const RibbonBatch& batch); // Replaces the group's lines and discs
    void setRibbonsVisible(RibbonGroup group, bool visible);     // Hides a group without dropping it
    void setArrows(ArrowGroup group, const ArrowBatch& batch);
    void setArrowsVisible(ArrowGroup group, bool visible);
    float arrowSlotPx = ARROW_TARGET_SPACING_PX; // Pixels between arrows, their size is a fraction of it
    void setDebugLines(const std::vector<DebugVertex>& lines);
    void setDebugPoints(const std::vector<DebugVertex>& points);
    bool wireframe = false; // Debug: the mesh's triangle edges over the cells
    void draw(const PlanetCamera& camera, const glm::vec2& viewport);

    bool ready() const { return indexCount > 0; }

private:
    // Uniform locations of the shared projection code, in one program
    struct ProjectionUniforms {
        int projection = -1, viewProj = -1, lift = -1, firstCopy = -1, maxY = -1, maxX = -1, eye = -1, horizonClip = -1, roundPoints = -1;
        void fetch(unsigned int program);
        // Needs the program in use
        void set(int projectionMode, const glm::mat4& viewProjMatrix, float liftValue, const glm::vec3& eyePosition,
            bool clipHorizon, float firstCopyValue, float maxYValue, float maxXValue) const;
    };
    struct RibbonLayer {
        unsigned int vao = 0, buffer = 0, indexBuffer = 0, discVao = 0, discBuffer = 0;
        int indexCount = 0, discCount = 0;
        float lonMin = 0.f, lonMax = 0.f;
        bool visible = true;
    };
    struct ArrowLayer {
        unsigned int vao = 0, buffer = 0;
        int vertexCount = 0;
        float lonMin = 0.f, lonMax = 0.f;
        bool visible = true;
    };

    void release();
    void upload(const PlanetMesh& mesh);
    bool ensurePrograms();
    void drawGlobe(const PlanetCamera& camera, const glm::vec2& viewport);
    void drawFlat(const PlanetCamera& camera, const glm::vec2& viewport);
    // Wireframe and debug overlays, with the mesh program's projection uniforms already set
    void drawOverlays(bool flat, int instances);
    // Ribbon groups [from, to), switches to the ribbon program and leaves it in use
    void drawRibbons(const PlanetCamera& camera, const glm::vec2& viewport, RibbonGroup from, RibbonGroup to);
    // Every arrow layer, switches to the arrow program and leaves it in use
    void drawArrows(const PlanetCamera& camera, const glm::vec2& viewport);
    void setDebugBuffer(unsigned int buffer, int& count, const std::vector<DebugVertex>& vertices);

    // GL object names, GLuint. They belong to the context that made them, and are only freed
    // while it is active (after the window closes they go with the process)
    unsigned long long contextId = 0;
    bool contextActive() const;
    unsigned int program = 0, ribbonProgram = 0, arrowProgram = 0;
    ProjectionUniforms meshUniforms, ribbonUniforms, arrowUniforms;
    int ribbonViewportLocation = -1, ribbonDiscsLocation = -1, arrowViewportLocation = -1, arrowSlotLocation = -1;
    unsigned int vao = 0, positionBuffer = 0, latLonBuffer = 0, colorBuffer = 0, indexBuffer = 0;
    unsigned int debugLineVao = 0, debugLineBuffer = 0, debugPointVao = 0, debugPointBuffer = 0;
    int debugLineCount = 0, debugPointCount = 0;
    RibbonLayer ribbons[RIBBON_GROUP_COUNT];
    ArrowLayer arrows[ARROW_GROUP_COUNT];

    int indexCount = 0;
    int poleFanIndexCount = 0;
    int globeIndexCount = 0;
    float lonMin = 0.f, lonMax = 0.f;
    std::vector<int> cellFirstVertex;
    std::vector<PlanetMesh::Band> poleBands;
    std::vector<sf::Color> vertexColors;
};
