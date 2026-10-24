#include "PlanetColors.hpp"
#include "util.hpp"

std::vector<sf::Color> mapModeColors(const std::vector<Cell>& cells, const GlobalWorldObjects& globals,
    const MapColors& colors, int mapType, ResourceType resource)
{
    const std::size_t n = cells.size();
    std::vector<sf::Color> out(n, sf::Color(80, 80, 80));

    switch (mapType) {
    case 0:
        for (std::size_t i = 0; i < n; i++) out[i] = heightColor(colors.height, cells[i].height, cells[i].oceanBool);
        break;
    case 1:
        for (std::size_t i = 0; i < n; i++) out[i] = temperatureColor(colors.temperature, cells[i].temp);
        break;
    case 2:
        for (std::size_t i = 0; i < n; i++) {
            const int biome = cells[i].biome;
            if (biome >= 0 && biome < static_cast<int>(globals.biomes.size())) out[i] = globals.biomes[biome].color;
        }
        break;
    case 3:
        for (std::size_t i = 0; i < n; i++) out[i] = precipitationColor(colors.precipitation, cells[i].percepitation);
        break;
    case 4:
        for (std::size_t i = 0; i < n; i++) {
            out[i] = sf::Color(0, 0, static_cast<sf::Uint8>(clamp(255 * cells[i].humidity, 255, 0)), 255);
        }
        break;
    case 5: {
        // New random colours every redraw, like the flat map
        ColorTable colorTable;
        std::vector<sf::Color> continentColors = colorTable.getRandomColors(static_cast<int>(globals.continents.size()));
        for (std::size_t i = 0; i < n; i++) {
            const int continent = cells[i].continent;
            if (continent >= 0 && continent < static_cast<int>(continentColors.size())) out[i] = continentColors[continent];
        }
        break;
    }
    case 7: {
        // Yellow to red by abundance, grey without the resource
        float maxAmount = 0.f;
        for (const Cell& cell : cells) maxAmount = std::max(maxAmount, cell.resources.getResourceAmount(resource));
        if (maxAmount < 0.001f) break;
        for (std::size_t i = 0; i < n; i++) {
            const float normalizedAmount = cells[i].resources.getResourceAmount(resource) / maxAmount;
            if (normalizedAmount >= 0.001f) {
                out[i] = sf::Color(255, static_cast<sf::Uint8>(255 * (1.0f - normalizedAmount)), 0, 255);
            }
        }
        break;
    }
    case 8:
        for (std::size_t i = 0; i < n; i++) {
            if (cells[i].oceanBool) out[i] = colors.water.ocean;
            else if (cells[i].lakeBool) out[i] = colors.water.lake;
            else out[i] = waterColor(colors.water, cells[i].discharge);
        }
        break;
    default: // Cultures have no colours yet
        break;
    }
    // Lakes are water, and snow and sea ice cover the terrain, in the modes that show it
    if (mapType == 0 || mapType == 2) {
        const SnowIceColors& snowIce = colors.snowIce;
        for (std::size_t i = 0; i < n; i++) {
            if (cells[i].lakeBool && !cells[i].oceanBool) { out[i] = colors.water.lake; continue; }
            if (!snowIce.enabled) continue;
            if (cells[i].snowBool) out[i] = tintColor(out[i], snowIce.snow, snowIce.snowStrength);
            else if (cells[i].iceBool) out[i] = tintColor(out[i], snowIce.ice, snowIce.iceStrength);
        }
    }
    return out;
}

std::vector<sf::Color> seasonalMapModeColors(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals,
    const MapColors& colors, int mapType, ResourceType resource, const SeasonalCalculator& seasons, float dayOfYear)
{
    if (mapType != 1 && mapType != 3 && mapType != 4) {
        return mapModeColors(planet.cells, globals, colors, mapType, resource);
    }

    const std::vector<Cell>& cells = planet.cells;
    std::vector<sf::Color> out(cells.size());
    for (std::size_t i = 0; i < cells.size(); i++) {
        const float lat = planet.cellLat[i];
        if (mapType == 1) {
            out[i] = temperatureColor(colors.temperature, seasons.getTemperature(cells[i], lat, dayOfYear));
        }
        else if (mapType == 3) {
            out[i] = precipitationColor(colors.precipitation, seasons.getPercepitationDistribution(cells[i], lat, dayOfYear).mean);
        }
        else {
            const float humidity = seasons.getHumidityDistribution(cells[i], lat, dayOfYear).mean;
            out[i] = sf::Color(0, 0, static_cast<sf::Uint8>(clamp(255 * humidity, 255, 0)), 255);
        }
    }
    return out;
}
