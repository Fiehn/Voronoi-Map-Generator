#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include "SphereVoronoi.hpp"
#include "PlanetCamera.hpp"
#include "GlobeRenderer.hpp"

// The planet on screen: camera, globe renderer, mouse input and cell picking.
// Left drag orbits, the wheel zooms towards the cursor, the cell under the cursor is highlighted.
//
// Until the simulation runs on the sphere the cells get debug colours (see colorMode).
class PlanetView {
public:
    PlanetCamera camera;
    int hoveredCell = -1;
    int colorMode = 0; // 0: random colour per cell, 1: lat/lon checkerboard

    // Settings for regenerating the preview planet
    int previewCells = 22500;
    float previewJitter = 0.5f;

    // After the planet is (re)generated. Needs the window's GL context active
    void rebuild(const vor::SphereVoronoi& planet);
    void recolor(const vor::SphereVoronoi& planet);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::RenderWindow& window, const vor::SphereVoronoi& planet); // Once per frame, picks the hovered cell
    void draw(sf::RenderWindow& window);

    // ImGui panel: regenerate, colour mode, camera and hovered cell readout
    void ui(vor::SphereVoronoi& planet, sf::RenderWindow& window);

    bool hasPlanet() const { return renderer.ready(); }

private:
    float maxZoom() const;

    GlobeRenderer renderer;
    bool dragging = false;
    glm::vec2 lastMouse{ 0.f };
    float cellSpacing = 0.f; // Radians, for the zoom limit
};

// Debug colours for the preview, one per cell
std::vector<sf::Color> debugCellColors(const vor::SphereVoronoi& planet, int mode);
