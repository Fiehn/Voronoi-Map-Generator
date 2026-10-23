#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include "SphereVoronoi.hpp"
#include "PlanetCamera.hpp"
#include "GlobeRenderer.hpp"
#include "GlobalWorldObjects.hpp"

// The planet on screen: camera, globe renderer, mouse input and cell picking.
// Left drag orbits, the wheel zooms towards the cursor, the cell under the cursor is highlighted.
//
// The cells show the map mode colours given to setCellColors, or debug colours when those are switched on.
class PlanetView {
public:
    PlanetCamera camera;
    int hoveredCell = -1;
    bool debugColors = false;
    int debugMode = 0; // 0: random colour per cell, 1: lat/lon checkerboard

    // After the planet is (re)generated. Needs the window's GL context active
    void rebuild(const vor::SphereVoronoi& planet);
    // Map mode colours, one per cell. Needs the window's GL context active
    void setCellColors(const vor::SphereVoronoi& planet, const std::vector<sf::Color>& colors);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::RenderWindow& window, const vor::SphereVoronoi& planet); // Once per frame, picks the hovered cell
    void draw(sf::RenderWindow& window);

    // ImGui panel: debug colours, camera and hovered cell readout
    void ui(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, sf::RenderWindow& window);

    bool hasPlanet() const { return renderer.ready(); }

private:
    float maxZoom() const;
    void uploadColors(const vor::SphereVoronoi& planet);

    std::vector<sf::Color> mapColors; // Kept so switching the debug colours off restores them
    GlobeRenderer renderer;
    bool dragging = false;
    glm::vec2 lastMouse{ 0.f };
    float cellSpacing = 0.f; // Radians, for the zoom limit
};

// Debug colours for the preview, one per cell
std::vector<sf::Color> debugCellColors(const vor::SphereVoronoi& planet, int mode);
