#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/Color.hpp>
#include "SphereVoronoi.hpp"
#include "PlanetCamera.hpp"

// Draws the planet's cells on the globe with raw OpenGL 3.3. Each cell is a fan from its
// centre around its ring, positions are static and colours are a separate buffer, so
// recolouring the map is one upload.
//
// Every call needs the window's GL context to be active. draw() leaves no VAO, program or
// buffer bound and depth testing off, so SFML and ImGui can draw after it once
// window.resetGLStates() has run.
class GlobeRenderer {
public:
    GlobeRenderer() = default;
    ~GlobeRenderer();
    GlobeRenderer(const GlobeRenderer&) = delete;
    GlobeRenderer& operator=(const GlobeRenderer&) = delete;

    void build(const vor::SphereVoronoi& sphere);
    void setCellColors(const std::vector<sf::Color>& colors); // One per cell
    void setHighlight(const vor::SphereVoronoi& sphere, int cell); // -1 hides it
    void draw(const PlanetCamera& camera, const glm::vec2& viewport);

    bool ready() const { return indexCount > 0; }

private:
    void release();

    // GL object names, GLuint. They belong to the context that made them, and are only freed
    // while it is active (after the window closes they go with the process)
    unsigned long long contextId = 0;
    bool contextActive() const;
    unsigned int program = 0;
    unsigned int vao = 0, positionBuffer = 0, colorBuffer = 0, indexBuffer = 0;
    unsigned int highlightVao = 0, highlightBuffer = 0;
    int viewProjLocation = -1, liftLocation = -1;

    int indexCount = 0;
    int highlightCount = 0;
    std::vector<int> cellFirstVertex; // Cell i owns mesh vertices [cellFirstVertex[i], cellFirstVertex[i + 1])
    std::vector<sf::Color> vertexColors;
};
