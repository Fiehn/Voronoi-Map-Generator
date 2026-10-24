#include "RenderSettings.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include "Logger.h"
#include "imgui.h"
#include "Include/nlohmann/json.hpp"

namespace {

    const char* BORDER_TYPE_NAMES[BORDER_TYPE_COUNT] = { "Coast", "Shore", "Continent", "Biome" };
    const char* MAP_MODE_NAMES[MAP_MODE_COUNT] = { "Height", "Temperature", "Biome", "Percepitation", "Humidity", "Continents", "Cultures", "Resources", "Water" };

    nlohmann::json colorToJson(const sf::Color& color)
    {
        return nlohmann::json::array({ color.r, color.g, color.b, color.a });
    }

    sf::Color colorFromJson(const nlohmann::json& j, const sf::Color& fallback)
    {
        if (!j.is_array() || j.size() != 4) { return fallback; }
        return sf::Color(j[0].get<sf::Uint8>(), j[1].get<sf::Uint8>(), j[2].get<sf::Uint8>(), j[3].get<sf::Uint8>());
    }

    // Every overlay style, for the file and the UI
    struct OverlayEntry
    {
        const char* label;
        const char* key;
        OverlayStyle OverlaySettings::* style;
    };
    const OverlayEntry OVERLAYS[] = {
        { "Highlighted cell", "highlight", &OverlaySettings::highlight },
        { "Found cells", "found_cells", &OverlaySettings::foundCells },
        { "Rivers (widest)", "rivers", &OverlaySettings::rivers },
        { "Convergence lines", "convergence_lines", &OverlaySettings::convergenceLines },
    };

    // ImGui colour editor for an sf::Color, true when it changed
    bool colorEdit(const char* label, sf::Color& color)
    {
        float c[4] = { color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f };
        if (!ImGui::ColorEdit4(label, c)) { return false; }
        color = sf::Color(
            static_cast<sf::Uint8>(c[0] * 255.f), static_cast<sf::Uint8>(c[1] * 255.f),
            static_cast<sf::Uint8>(c[2] * 255.f), static_cast<sf::Uint8>(c[3] * 255.f));
        return true;
    }
}

// ---------------------------------------------------------------------------------------------
// RenderSettings

RenderSettings::RenderSettings()
{
    BoundaryStyle& coast = styles[static_cast<int>(BorderType::Coast)];
    coast.subdivisions = 6;
    coast.strength = 1.f;
    coast.outlineWidthPx = 2.f;
    coast.outlineColor = sf::Color(25, 30, 40, 255);

    BoundaryStyle& shore = styles[static_cast<int>(BorderType::Shore)];
    shore.subdivisions = 6;
    shore.strength = 1.f;
    shore.outlineWidthPx = 1.5f;
    shore.outlineColor = sf::Color(25, 55, 110, 255);

    BoundaryStyle& continent = styles[static_cast<int>(BorderType::Continent)];
    continent.subdivisions = 6;
    continent.strength = 1.f;
    continent.outlineWidthPx = 2.5f;
    continent.outlineColor = sf::Color(170, 40, 40, 255);

    BoundaryStyle& biome = styles[static_cast<int>(BorderType::Biome)];
    biome.subdivisions = 4;
    biome.strength = 0.8f;
    biome.outlineWidthPx = 1.f;
    biome.outlineColor = sf::Color(30, 30, 30, 120);

    // Coast in every mode, plus the mode's own categorical boundary
    for (int mode = 0; mode < MAP_MODE_COUNT; mode++)
    {
        modeTypes[mode][static_cast<int>(BorderType::Coast)] = true;
        modeTypes[mode][static_cast<int>(BorderType::Shore)] = true;
    }
    modeTypes[2][static_cast<int>(BorderType::Biome)] = true;
    modeTypes[5][static_cast<int>(BorderType::Continent)] = true;
}

void RenderSettings::save_json(const std::string& filename) const
{
    nlohmann::json j;
    nlohmann::json jStyles = nlohmann::json::array();
    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        const BoundaryStyle& style = styles[t];
        jStyles.push_back({
            {"name", BORDER_TYPE_NAMES[t]},
            {"subdivisions", style.subdivisions},
            {"strength", style.strength},
            {"outline", style.outline},
            {"outline_width_px", style.outlineWidthPx},
            {"outline_color", colorToJson(style.outlineColor)}
            });
    }
    j["styles"] = jStyles;

    nlohmann::json jModes = nlohmann::json::array();
    for (int mode = 0; mode < MAP_MODE_COUNT; mode++)
    {
        nlohmann::json jMode = nlohmann::json::array();
        for (int t = 0; t < BORDER_TYPE_COUNT; t++) { jMode.push_back(modeTypes[mode][t]); }
        jModes.push_back(jMode);
    }
    j["mode_types"] = jModes;

    const HeightColors& height = colors.height;
    const TemperatureColors& temperature = colors.temperature;
    const PrecipitationColors& precipitation = colors.precipitation;
    j["map_colors"] = {
        {"height", {
            {"land", colorToJson(height.land)},
            {"ocean", colorToJson(height.ocean)},
            {"overflow", colorToJson(height.overflow)},
            {"alpha_floor", height.alphaFloor},
            {"steps_per_unit", height.stepsPerUnit}
        }},
        {"temperature", {
            {"cold", colorToJson(temperature.cold)},
            {"hot", colorToJson(temperature.hot)},
            {"cold_overflow", colorToJson(temperature.coldOverflow)},
            {"hot_overflow", colorToJson(temperature.hotOverflow)},
            {"steps_per_degree", temperature.stepsPerDegree}
        }},
        {"precipitation", {
            {"dry", colorToJson(precipitation.dry)},
            {"wet", colorToJson(precipitation.wet)},
            {"overflow", colorToJson(precipitation.overflow)},
            {"steps_per_unit", precipitation.stepsPerUnit}
        }}
    };

    j["map_colors"]["water"] = {
        {"lake", colorToJson(colors.water.lake)},
        {"dry", colorToJson(colors.water.dry)},
        {"wet", colorToJson(colors.water.wet)},
        {"ocean", colorToJson(colors.water.ocean)}
    };

    j["map_colors"]["snow_ice"] = {
        {"enabled", colors.snowIce.enabled},
        {"snow", colorToJson(colors.snowIce.snow)},
        {"ice", colorToJson(colors.snowIce.ice)},
        {"snow_strength", colors.snowIce.snowStrength},
        {"ice_strength", colors.snowIce.iceStrength}
    };

    nlohmann::json jOverlays = nlohmann::json::object();
    for (const OverlayEntry& entry : OVERLAYS)
    {
        const OverlayStyle& style = overlays.*entry.style;
        jOverlays[entry.key] = { {"color", colorToJson(style.color)}, {"width_px", style.widthPx} };
    }
    jOverlays["river_min_width_px"] = overlays.riverMinWidthPx;
    j["overlays"] = jOverlays;

    std::ofstream o(filename);
    o << std::setw(4) << j << std::endl;
}

bool RenderSettings::load_json(const std::string& filename)
{
    std::ifstream i(filename);
    if (!i.is_open()) { return false; }

    nlohmann::json j;
    try { i >> j; }
    catch (const nlohmann::json::exception& e)
    {
        LOG_WARNING(Graphics, "Could not parse render settings", filename, std::string(e.what()));
        return false;
    }

    // Styles are matched by name and mode flags by position, files from before the Shore type have one entry fewer
    const nlohmann::json& jStyles = j.value("styles", nlohmann::json::array());
    for (const nlohmann::json& js : jStyles)
    {
        int t = -1;
        for (int k = 0; k < BORDER_TYPE_COUNT; k++)
        {
            if (js.value("name", std::string()) == BORDER_TYPE_NAMES[k]) { t = k; }
        }
        if (t < 0) { continue; }
        BoundaryStyle& style = styles[t];
        style.subdivisions = std::clamp(js.value("subdivisions", style.subdivisions), 1, 16);
        style.strength = std::clamp(js.value("strength", style.strength), 0.f, 1.f);
        style.outline = js.value("outline", style.outline);
        style.outlineWidthPx = std::clamp(js.value("outline_width_px", style.outlineWidthPx), 0.5f, 8.f);
        style.outlineColor = colorFromJson(js.value("outline_color", nlohmann::json()), style.outlineColor);
    }

    const nlohmann::json& jModes = j.value("mode_types", nlohmann::json::array());
    for (int mode = 0; mode < MAP_MODE_COUNT && mode < static_cast<int>(jModes.size()); mode++)
    {
        const bool oldFormat = jModes[mode].size() == BORDER_TYPE_COUNT - 1; // Coast, Continent, Biome
        for (int t = 0; t < BORDER_TYPE_COUNT && t < static_cast<int>(jModes[mode].size()); t++)
        {
            const int type = oldFormat && t >= static_cast<int>(BorderType::Shore) ? t + 1 : t;
            if (jModes[mode][t].is_boolean()) { modeTypes[mode][type] = jModes[mode][t].get<bool>(); }
        }
    }

    const nlohmann::json& jColors = j.value("map_colors", nlohmann::json::object());

    const nlohmann::json& jHeight = jColors.value("height", nlohmann::json::object());
    HeightColors& height = colors.height;
    height.land = colorFromJson(jHeight.value("land", nlohmann::json()), height.land);
    height.ocean = colorFromJson(jHeight.value("ocean", nlohmann::json()), height.ocean);
    height.overflow = colorFromJson(jHeight.value("overflow", nlohmann::json()), height.overflow);
    height.alphaFloor = std::clamp(jHeight.value("alpha_floor", height.alphaFloor), 0, 255);
    height.stepsPerUnit = std::clamp(jHeight.value("steps_per_unit", height.stepsPerUnit), 1.f, 10000.f);

    const nlohmann::json& jTemperature = jColors.value("temperature", nlohmann::json::object());
    TemperatureColors& temperature = colors.temperature;
    temperature.cold = colorFromJson(jTemperature.value("cold", nlohmann::json()), temperature.cold);
    temperature.hot = colorFromJson(jTemperature.value("hot", nlohmann::json()), temperature.hot);
    temperature.coldOverflow = colorFromJson(jTemperature.value("cold_overflow", nlohmann::json()), temperature.coldOverflow);
    temperature.hotOverflow = colorFromJson(jTemperature.value("hot_overflow", nlohmann::json()), temperature.hotOverflow);
    temperature.stepsPerDegree = std::clamp(jTemperature.value("steps_per_degree", temperature.stepsPerDegree), 0.1f, 10000.f);

    const nlohmann::json& jPrecipitation = jColors.value("precipitation", nlohmann::json::object());
    PrecipitationColors& precipitation = colors.precipitation;
    precipitation.dry = colorFromJson(jPrecipitation.value("dry", nlohmann::json()), precipitation.dry);
    precipitation.wet = colorFromJson(jPrecipitation.value("wet", nlohmann::json()), precipitation.wet);
    precipitation.overflow = colorFromJson(jPrecipitation.value("overflow", nlohmann::json()), precipitation.overflow);
    precipitation.stepsPerUnit = std::clamp(jPrecipitation.value("steps_per_unit", precipitation.stepsPerUnit), 0.01f, 10000.f);

    const nlohmann::json& jWater = jColors.value("water", nlohmann::json::object());
    colors.water.lake = colorFromJson(jWater.value("lake", nlohmann::json()), colors.water.lake);
    colors.water.dry = colorFromJson(jWater.value("dry", nlohmann::json()), colors.water.dry);
    colors.water.wet = colorFromJson(jWater.value("wet", nlohmann::json()), colors.water.wet);
    colors.water.ocean = colorFromJson(jWater.value("ocean", nlohmann::json()), colors.water.ocean);

    const nlohmann::json& jSnowIce = jColors.value("snow_ice", nlohmann::json::object());
    colors.snowIce.enabled = jSnowIce.value("enabled", colors.snowIce.enabled);
    colors.snowIce.snow = colorFromJson(jSnowIce.value("snow", nlohmann::json()), colors.snowIce.snow);
    colors.snowIce.ice = colorFromJson(jSnowIce.value("ice", nlohmann::json()), colors.snowIce.ice);
    colors.snowIce.snowStrength = std::clamp(jSnowIce.value("snow_strength", colors.snowIce.snowStrength), 0.f, 1.f);
    colors.snowIce.iceStrength = std::clamp(jSnowIce.value("ice_strength", colors.snowIce.iceStrength), 0.f, 1.f);

    const nlohmann::json& jOverlays = j.value("overlays", nlohmann::json::object());
    overlays.riverMinWidthPx = std::clamp(jOverlays.value("river_min_width_px", overlays.riverMinWidthPx), 0.5f, 8.f);
    for (const OverlayEntry& entry : OVERLAYS)
    {
        if (!jOverlays.contains(entry.key)) { continue; }
        const nlohmann::json& jo = jOverlays.at(entry.key);
        OverlayStyle& style = overlays.*entry.style;
        style.color = colorFromJson(jo.value("color", nlohmann::json()), style.color);
        style.widthPx = std::clamp(jo.value("width_px", style.widthPx), 0.5f, 8.f);
    }

    geometryRevision++;
    styleRevision++;
    return true;
}

// ---------------------------------------------------------------------------------------------
// UI

bool boundarySettingsUI(RenderSettings& settings, int mapMode)
{
    if (!ImGui::CollapsingHeader("Boundaries")) { return false; }

    bool geometryChanged = false;
    mapMode = std::clamp(mapMode, 0, MAP_MODE_COUNT - 1);

    // Own ID scope, the tree nodes below use the same labels
    ImGui::Text("Smoothed in %s mode:", MAP_MODE_NAMES[mapMode]);
    ImGui::PushID("ModeTypes");
    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        if (t > 0) { ImGui::SameLine(); }
        if (ImGui::Checkbox(BORDER_TYPE_NAMES[t], &settings.modeTypes[mapMode][t])) { geometryChanged = true; }
    }
    ImGui::PopID();

    for (int t = 0; t < BORDER_TYPE_COUNT; t++)
    {
        ImGui::PushID(t);
        if (ImGui::TreeNode(BORDER_TYPE_NAMES[t]))
        {
            BoundaryStyle& style = settings.styles[t];

            // Geometry rebuilds when the slider is released
            ImGui::SliderInt("Subdivisions", &style.subdivisions, 1, 16);
            if (ImGui::IsItemDeactivatedAfterEdit()) { geometryChanged = true; }
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Segments per boundary edge"); ImGui::EndTooltip(); }

            ImGui::SliderFloat("Strength", &style.strength, 0.f, 1.f, "%.2f");
            if (ImGui::IsItemDeactivatedAfterEdit()) { geometryChanged = true; }
            if (ImGui::BeginItemTooltip()) { ImGui::Text("0 is the original Voronoi edges, 1 is the full curve"); ImGui::EndTooltip(); }

            // Outlines are cheap to rebuild, so they update live
            if (ImGui::Checkbox("Outline", &style.outline)) { settings.styleRevision++; }
            if (ImGui::SliderFloat("Width (px)", &style.outlineWidthPx, 0.5f, 8.f, "%.1f")) { settings.styleRevision++; }
            if (colorEdit("Color", style.outlineColor)) { settings.styleRevision++; }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    if (ImGui::TreeNode("Debug"))
    {
        if (ImGui::Checkbox("Wireframe", &settings.debugWireframe)) { settings.styleRevision++; }
        if (ImGui::Checkbox("Chains and pinned vertices", &settings.debugChains)) { settings.styleRevision++; }
        if (ImGui::Checkbox("Clamped vertices", &settings.debugClamped)) { settings.styleRevision++; }
        if (ImGui::Checkbox("Original edges", &settings.debugOriginalEdges)) { settings.styleRevision++; }
        ImGui::TreePop();
    }

    if (geometryChanged) { settings.geometryRevision++; }
    return geometryChanged;
}

void overlaySettingsUI(RenderSettings& settings)
{
    if (!ImGui::CollapsingHeader("Overlays")) { return; }

    ImGui::PushID("Overlays");
    for (const OverlayEntry& entry : OVERLAYS)
    {
        if (!ImGui::TreeNode(entry.label)) { continue; }
        OverlayStyle& style = settings.overlays.*entry.style;
        if (ImGui::SliderFloat("Width (px)", &style.widthPx, 0.5f, 8.f, "%.1f")) { settings.styleRevision++; }
        if (colorEdit("Color", style.color)) { settings.styleRevision++; }
        if (entry.style == &OverlaySettings::rivers)
        {
            if (ImGui::SliderFloat("Smallest river (px)", &settings.overlays.riverMinWidthPx, 0.5f, 8.f, "%.1f")) { settings.styleRevision++; }
            if (ImGui::BeginItemTooltip()) { ImGui::Text("Rivers are drawn between the smallest and the widest, by the square root of their discharge"); ImGui::EndTooltip(); }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

bool renderSettingsFileUI(RenderSettings& settings)
{
    if (ImGui::Button("Save Style")) { settings.save_json(RENDER_SETTINGS_FILE); }
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Saves the boundaries and map colours"); ImGui::EndTooltip(); }
    ImGui::SameLine();
    if (ImGui::Button("Load Style"))
    {
        if (settings.load_json(RENDER_SETTINGS_FILE)) { return true; }
        LOG_WARNING(Graphics, "Could not load render settings", std::string(RENDER_SETTINGS_FILE));
    }
    return false;
}
