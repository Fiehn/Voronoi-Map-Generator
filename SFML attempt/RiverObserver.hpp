#pragma once
#include <vector>
#include "SphereVoronoi.hpp"
#include "GlobalWorldObjects.hpp"
#include "PlanetView.hpp"

// ImGui window listing every river and lake of the hydrology step with their stats. The tables sort
// by any column, clicking a row selects it, outlines its cells on the map and moves the camera to it,
// and the details underneath link on to the tributaries, the parent, the lake a river ends in and so on
class RiverObserver {
public:
    bool open = false;

    // Cells to outline on the map for the selection, for the caller to hand to the planet view
    const std::vector<std::size_t>& highlight() const { return highlightCells; }

    // After a new world, the selection points into the old one
    void reset();

    void ui(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window);

private:
    void selectRiver(int id, const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window);
    void selectLake(int id, const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window);
    void riversTab(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window);
    void lakesTab(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window);

    std::vector<std::size_t> highlightCells;
    int selectedRiver = -1;
    int selectedLake = -1;
    bool mainRiversOnly = false;
    int lakeFilter = 0;     // 0 all, 1 draining, 2 closed
    int pendingTab = -1;    // 0 rivers, 1 lakes: a link in the details asked for this tab
};
