#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/Color.hpp>
#include "SphereVoronoi.hpp"
#include "PlanetCamera.hpp"

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

PlanetMesh buildPlanetMesh(const vor::SphereVoronoi& sphere);

// Outline of a cell as a line strip for both views: the ring with its first vertex repeated at
// the end. The longitude starts unwrapped around the cell centre, like the mesh, and then
// follows each step from the one before, so a pole cell's outline goes once round in longitude
// and the flat view draws it as one line along the bottom of its band.
struct OutlineVertex {
    glm::vec3 position;
    glm::vec2 latLon; // Radians
};
std::vector<OutlineVertex> buildCellOutline(const vor::SphereVoronoi& sphere, int cell);

// Draws the planet's cells with raw OpenGL 3.3, on the globe or as a flat map depending on the
// camera's mode. Positions are static and colours are a separate buffer, so recolouring the
// map is one upload.
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

    void build(const vor::SphereVoronoi& sphere);
    void setCellColors(const std::vector<sf::Color>& colors); // One per cell
    void setHighlight(const vor::SphereVoronoi& sphere, int cell); // Outline of the cell, -1 hides it
    void draw(const PlanetCamera& camera, const glm::vec2& viewport);

    bool ready() const { return indexCount > 0; }

private:
    void release();
    void drawGlobe(const PlanetCamera& camera, const glm::vec2& viewport);
    void drawFlat(const PlanetCamera& camera, const glm::vec2& viewport);

    // GL object names, GLuint. They belong to the context that made them, and are only freed
    // while it is active (after the window closes they go with the process)
    unsigned long long contextId = 0;
    bool contextActive() const;
    unsigned int program = 0;
    unsigned int vao = 0, positionBuffer = 0, latLonBuffer = 0, colorBuffer = 0, indexBuffer = 0;
    unsigned int highlightVao = 0, highlightBuffer = 0;
    int viewProjLocation = -1, liftLocation = -1, projectionLocation = -1, firstCopyLocation = -1, maxYLocation = -1, maxXLocation = -1;

    int indexCount = 0;
    int poleFanIndexCount = 0;
    int globeIndexCount = 0;
    float lonMin = 0.f, lonMax = 0.f;
    int highlightCount = 0;
    std::vector<int> cellFirstVertex;
    std::vector<PlanetMesh::Band> poleBands;
    std::vector<sf::Color> vertexColors;
};
