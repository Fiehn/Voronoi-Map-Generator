#pragma once
#include <vector>
#include <SFML/Graphics/Color.hpp>
#include "cell.hpp"
#include "GlobalWorldObjects.hpp"
#include "MapColors.hpp"
#include "SeasonalCalculator.hpp"
#include "SphereVoronoi.hpp"

// One colour per cell for a map mode, the same colours the flat map's draw*Map functions gave it.
// mapType: 0 height, 1 temperature, 2 biome, 3 precipitation, 4 humidity, 5 continents, 6 cultures, 7 resources
std::vector<sf::Color> mapModeColors(const std::vector<Cell>& cells, const GlobalWorldObjects& globals,
    const MapColors& colors, int mapType, ResourceType resource);

// The temperature, precipitation and humidity maps on a day of the year. Other map types get mapModeColors
std::vector<sf::Color> seasonalMapModeColors(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals,
    const MapColors& colors, int mapType, ResourceType resource, const SeasonalCalculator& seasons, float dayOfYear);
