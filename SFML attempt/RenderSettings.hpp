#pragma once
#include <string>
#include <SFML/Graphics/Color.hpp>
#include "MapColors.hpp"

// Render settings: boundary smoothing and outlines per map mode, and the map colours.
// Kept apart from the map config and saved to their own file

enum class BorderType : int
{
    // Ordered by priority, an edge gets the first enabled type whose labels differ across it
    Coast = 0,  // oceanBool differs
    Shore,      // lakeBool differs, a lake edge
    Continent,  // continent differs
    // TODO: Culture boundaries (cell.culture) once the Cultures map is implemented.
    Biome,      // biome differs
    Count
};

constexpr int BORDER_TYPE_COUNT = static_cast<int>(BorderType::Count);
constexpr int MAP_MODE_COUNT = 9; // Matches the mapType radio buttons in main.cpp
inline constexpr const char* RENDER_SETTINGS_FILE = "render_settings.json";

struct BoundaryStyle
{
    int subdivisions = 6; // Segments per boundary edge, 1 only moves the corners
    float strength = 1.f; // 0 is the original Voronoi edges and 1 is the full curve
    bool outline = true;
    float outlineWidthPx = 2.f; // Screen pixels, the outline keeps its width when zooming
    sf::Color outlineColor = sf::Color(20, 20, 20, 255);
};

// A line drawn over the map, constant width in screen pixels
struct OverlayStyle
{
    sf::Color color;
    float widthPx;
};

struct OverlaySettings
{
    OverlayStyle highlight{ sf::Color::White, 2.f };  // Cell under the cursor
    OverlayStyle foundCells{ sf::Color::Red, 2.f };   // Cells in the Find Cell list
    OverlayStyle rivers{ sf::Color::Blue, 6.f }; // The width is the biggest river's
    float riverMinWidthPx = 1.f; // The smallest river's
    OverlayStyle convergenceLines{ sf::Color::Green, 1.5f }; // Wind convergence zones
};

struct RenderSettings
{
    BoundaryStyle styles[BORDER_TYPE_COUNT];
    bool modeTypes[MAP_MODE_COUNT][BORDER_TYPE_COUNT] = {}; // Active boundary types for each map mode
    MapColors colors; // Height, temperature and precipitation colours
    OverlaySettings overlays;

    // Debug overlays (not saved)
    bool debugWireframe = false;
    bool debugChains = false;
    bool debugClamped = false;
    bool debugOriginalEdges = false;

    // Bumped on change so the renderer knows what to rebuild (not saved)
    unsigned int geometryRevision = 0;
    unsigned int styleRevision = 0;

    RenderSettings();

    void save_json(const std::string& filename) const;
    bool load_json(const std::string& filename); // Missing keys keep their defaults, returns false if the file can't be read
};

// Boundaries section for the Map Controls window, returns true when the geometry has to be rebuilt
bool boundarySettingsUI(RenderSettings& settings, int mapMode);

// Overlays section for the Map Controls window, bumps styleRevision on change
void overlaySettingsUI(RenderSettings& settings);

// Save and load buttons for the whole render settings file, returns true when the settings were loaded
bool renderSettingsFileUI(RenderSettings& settings);
