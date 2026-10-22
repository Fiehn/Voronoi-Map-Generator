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

struct MapColors
{
    HeightColors height;
    TemperatureColors temperature;
    PrecipitationColors precipitation;
};

sf::Color heightColor(const HeightColors& colors, float height, bool ocean);
sf::Color temperatureColor(const TemperatureColors& colors, float temperature);
sf::Color precipitationColor(const PrecipitationColors& colors, float precipitation);

// Map Colours section for the Map Controls window, returns true when a colour changed and the map has to be redrawn
bool mapColorsUI(MapColors& colors);
