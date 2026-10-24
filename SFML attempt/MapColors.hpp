#pragma once
#include <SFML/Graphics/Color.hpp>

// Colours for the value map modes (height, temperature, precipitation)
// Every mode walks a normal range one colour step at a time. Values are not capped, so past either end
// the remaining steps keep going at the same rate, zigzagging one channel at a time toward an overflow
// target. Nothing wraps or plateaus until the target is reached, and the same value is the same colour on every map

struct HeightColors
{
    sf::Color land = sf::Color(128, 255, 148);
    sf::Color ocean = sf::Color(0, 0, 233);
    sf::Color overflow = sf::Color(255, 255, 255); // Reached after the alpha is maxed
    int alphaFloor = 55; // Alpha at height 0, rises one step at a time to 255
    float stepsPerUnit = 200.f; // Colour steps per height unit
};

struct TemperatureColors
{
    sf::Color cold = sf::Color(255, 0, 0); // End of the normal range below 0C
    sf::Color hot = sf::Color(255, 255, 0); // End of the normal range above 0C
    sf::Color coldOverflow = sf::Color(0, 0, 255);
    sf::Color hotOverflow = sf::Color(255, 255, 255);
    float stepsPerDegree = 5.f; // 0C sits in the middle of the normal range
};

struct PrecipitationColors
{
    sf::Color dry = sf::Color(0, 0, 0);
    sf::Color wet = sf::Color(0, 255, 0);
    sf::Color overflow = sf::Color(255, 255, 255);
    float stepsPerUnit = 2.55f;
};

// Lakes in the terrain modes, and the Water mode: land from dry to wet by the discharge of its water (log scale)
struct WaterColors
{
    sf::Color lake = sf::Color(70, 130, 205);
    sf::Color dry = sf::Color(232, 226, 205);
    sf::Color wet = sf::Color(10, 45, 150);
    sf::Color ocean = sf::Color(85, 85, 85);
    float fullDischarge = 10000.f; // m3/s that has the wet colour
};

// Permanent snow on land and sea ice, mixed over the terrain colours of the Height and Biome modes
struct SnowIceColors
{
    bool enabled = true;
    sf::Color snow = sf::Color(250, 252, 255);
    sf::Color ice = sf::Color(205, 230, 245);
    float snowStrength = 0.85f; // 0 is the terrain colour, 1 is the snow colour
    float iceStrength = 0.7f;
};

struct MapColors
{
    HeightColors height;
    TemperatureColors temperature;
    PrecipitationColors precipitation;
    WaterColors water;
    SnowIceColors snowIce;
};

sf::Color heightColor(const HeightColors& colors, float height, bool ocean);
sf::Color temperatureColor(const TemperatureColors& colors, float temperature);
// base mixed toward target, strength 0 to 1
sf::Color tintColor(sf::Color base, sf::Color target, float strength);
sf::Color waterColor(const WaterColors& colors, float discharge);
sf::Color precipitationColor(const PrecipitationColors& colors, float precipitation);

// Map Colours section for the Map Controls window, returns true when a colour changed and the map has to be redrawn
bool mapColorsUI(MapColors& colors);
