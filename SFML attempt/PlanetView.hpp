#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include "SphereVoronoi.hpp"
#include "PlanetCamera.hpp"
#include "GlobeRenderer.hpp"
#include "SphereBoundary.hpp"
#include "RenderSettings.hpp"
#include "GlobalWorldObjects.hpp"

// The planet on screen: camera, globe renderer, mouse input and cell picking.
// Left drag orbits the globe or pans the flat map, the wheel zooms towards the cursor (and in
// Auto switches between the two), the cell under the cursor is highlighted.
//
// The cells show the map mode colours given to setCellColors, or debug colours when those are
// switched on, with the boundaries smoothed as the render settings ask for the map mode.
class PlanetView {
public:
    PlanetCamera camera;
    int hoveredCell = -1;
    bool debugColors = false;
    int debugMode = 0; // 0: random colour per cell, 1: lat/lon checkerboard
    bool showRivers = true;
    bool showConvergenceLines = false;

    // After the planet is (re)generated. Needs the window's GL context active
    void rebuild(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode);
    // Map mode colours, one per cell. Needs the window's GL context active
    void setCellColors(const vor::SphereVoronoi& planet, const std::vector<sf::Color>& colors, const RenderSettings& settings, int mapMode);
    // After the render settings changed, keeps the colours. Needs the window's GL context active
    void updateBoundaries(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode);
    // Cells in the Find Cell list, outlined. Cheap when the list is unchanged. Needs the window's GL context active
    void setFoundCells(const vor::SphereVoronoi& planet, const std::vector<std::size_t>& cells);
    // Rivers and convergence lines of a newly generated world. Needs the window's GL context active
    void setWorldLines(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(const sf::RenderWindow& window, const vor::SphereVoronoi& planet); // Once per frame, picks the hovered cell
    void draw(sf::RenderWindow& window);

    // ImGui panel: debug colours, camera and hovered cell readout
    void ui(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, sf::RenderWindow& window);

    bool hasPlanet() const { return renderer.ready(); }

private:
    float maxZoom() const;
    void uploadColors(const vor::SphereVoronoi& planet);
    // Boundary outlines, the highlight, the found cells and the debug overlays
    void updateOverlays(const vor::SphereVoronoi& planet, const RenderSettings& settings);
    void updateDebugOverlays(const vor::SphereVoronoi& planet, const RenderSettings& settings);
    void uploadHighlight(const vor::SphereVoronoi& planet);
    void uploadFoundCells(const vor::SphereVoronoi& planet);
    void uploadWorldLines();

    std::vector<sf::Color> mapColors; // Kept so switching the debug colours off restores them
    GlobeRenderer renderer;
    SphereBoundaries boundaries;
    OverlaySettings overlayStyles; // Of the last updateOverlays, for the highlight and found cells
    std::vector<std::size_t> foundCells;
    std::vector<std::vector<glm::vec3>> rivers; // Kept to rebuild the ribbons after a style change or a new mesh
    std::vector<float> convergenceLats;
    unsigned int overlayStyleRevision = 0;
    bool overlaysStale = true; // The mesh was rebuilt, which drops every overlay
    bool dragging = false;
    glm::vec2 lastMouse{ 0.f };
    float cellSpacing = 0.f; // Radians, for the zoom limit
};

// Debug colours for the preview, one per cell
std::vector<sf::Color> debugCellColors(const vor::SphereVoronoi& planet, int mode);
