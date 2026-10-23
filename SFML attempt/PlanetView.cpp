#include "PlanetView.hpp"
#include "Logger.h"
#include "imgui.h"
#include "SphereMath.hpp"
#include <limits>
#include <algorithm>
#include <cmath>
#include <string>
#include <cstdint>
#include <glm/gtc/constants.hpp>

namespace {
    constexpr float PIXELS_PER_CELL_AT_MAX_ZOOM = 400.f;
    constexpr float WHEEL_STEP = 1.15f;

    glm::vec2 viewportOf(const sf::RenderWindow& window)
    {
        return glm::vec2(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    }
}

std::vector<sf::Color> debugCellColors(const vor::SphereVoronoi& planet, int mode)
{
    const int n = static_cast<int>(planet.cells.size());
    std::vector<sf::Color> colors(n);
    for (int i = 0; i < n; i++) {
        if (mode == 0) {
            // Integer hash, so neighbouring cells get unrelated colours
            std::uint32_t h = static_cast<std::uint32_t>(i) * 2654435761u;
            h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
            colors[i] = sf::Color(60 + (h & 0xFF) * 3 / 4, 60 + ((h >> 8) & 0xFF) * 3 / 4, 60 + ((h >> 16) & 0xFF) * 3 / 4);
            continue;
        }

        // 15 degree checkerboard, warm in the north and cool in the south. The prime meridian is
        // white and 90E is red, so looking at lon 0 the red line must be to the right
        const float lat = planet.cellLat[i];
        const float lon = planet.cellLon[i];
        if (std::abs(lon) < 1.5f) { colors[i] = sf::Color::White; continue; }
        if (std::abs(lon - 90.f) < 1.5f) { colors[i] = sf::Color(220, 30, 30); continue; }
        const bool light = (static_cast<int>(std::floor((lat + 90.f) / 15.f)) + static_cast<int>(std::floor((lon + 180.f) / 15.f))) % 2 == 0;
        if (lat >= 0.f) colors[i] = light ? sf::Color(235, 170, 70) : sf::Color(170, 105, 30);
        else colors[i] = light ? sf::Color(90, 160, 230) : sf::Color(30, 85, 165);
    }
    return colors;
}

void PlanetView::rebuild(const vor::SphereVoronoi& planet)
{
    renderer.build(planet);
    cellSpacing = planet.cells.empty() ? 0.f : std::sqrt(4.f * glm::pi<float>() / planet.cells.size());
    hoveredCell = -1;
    uploadColors(planet);
}

void PlanetView::setCellColors(const vor::SphereVoronoi& planet, const std::vector<sf::Color>& colors)
{
    mapColors = colors;
    uploadColors(planet);
}

void PlanetView::uploadColors(const vor::SphereVoronoi& planet)
{
    if (debugColors) renderer.setCellColors(debugCellColors(planet, debugMode));
    else if (mapColors.size() == planet.cells.size()) renderer.setCellColors(mapColors);
}

float PlanetView::maxZoom() const
{
    return cellSpacing > 0.f ? PIXELS_PER_CELL_AT_MAX_ZOOM / cellSpacing : camera.zoom;
}

void PlanetView::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    const bool overUi = ImGui::GetIO().WantCaptureMouse;

    switch (event.type) {
    case sf::Event::MouseButtonPressed:
        if (event.mouseButton.button == sf::Mouse::Left && !overUi) {
            dragging = true;
            lastMouse = glm::vec2(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        }
        break;

    case sf::Event::MouseButtonReleased:
        if (event.mouseButton.button == sf::Mouse::Left) dragging = false;
        break;

    case sf::Event::MouseMoved:
        if (dragging) {
            const glm::vec2 mouse(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
            camera.drag(mouse - lastMouse);
            lastMouse = mouse;
        }
        break;

    case sf::Event::MouseWheelScrolled:
        if (!overUi && !dragging) {
            const glm::vec2 mouse(static_cast<float>(event.mouseWheelScroll.x), static_cast<float>(event.mouseWheelScroll.y));
            camera.zoomAt(std::pow(WHEEL_STEP, event.mouseWheelScroll.delta), mouse, viewportOf(window), maxZoom());
        }
        break;

    default:
        break;
    }
}

void PlanetView::update(const sf::RenderWindow& window, const vor::SphereVoronoi& planet)
{
    const glm::vec2 viewport = viewportOf(window);
    camera.clampTo(viewport, maxZoom());

    // Over the UI the last hovered cell stays, so its readout can still be looked at
    if (!hasPlanet() || ImGui::GetIO().WantCaptureMouse) return;

    const sf::Vector2i mouse = sf::Mouse::getPosition(window);
    int cell = -1;
    glm::vec3 hit;
    if (mouse.x >= 0 && mouse.y >= 0 && mouse.x < static_cast<int>(viewport.x) && mouse.y < static_cast<int>(viewport.y)
        && camera.pick(glm::vec2(static_cast<float>(mouse.x), static_cast<float>(mouse.y)), viewport, hit)) {
        cell = planet.findCell(hit, hoveredCell);
    }
    if (cell != hoveredCell) {
        hoveredCell = cell;
        renderer.setHighlight(planet, cell);
    }
}

void PlanetView::draw(sf::RenderWindow& window)
{
    if (!window.setActive(true)) return;
    renderer.draw(camera, viewportOf(window));
    window.resetGLStates();
}

void PlanetView::ui(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, sf::RenderWindow& window)
{
    ImGui::Begin("Planet view");

    const bool oldDebug = debugColors;
    const int oldMode = debugMode;
    ImGui::Checkbox("Debug colours", &debugColors);
    if (debugColors) {
        ImGui::SameLine();
        ImGui::RadioButton("Cells", &debugMode, 0); ImGui::SameLine();
        ImGui::RadioButton("Lat/Lon grid", &debugMode, 1);
        if (ImGui::BeginItemTooltip()) { ImGui::Text("White: prime meridian, red: 90E, warm: north, cool: south"); ImGui::EndTooltip(); }
    }
    if ((debugColors != oldDebug || debugMode != oldMode) && window.setActive(true)) uploadColors(planet);

    const glm::vec2 viewport = viewportOf(window);
    int lock = static_cast<int>(camera.lock);
    ImGui::Text("View"); ImGui::SameLine();
    ImGui::RadioButton("Auto", &lock, static_cast<int>(ViewLock::Auto)); ImGui::SameLine();
    ImGui::RadioButton("Globe", &lock, static_cast<int>(ViewLock::Globe)); ImGui::SameLine();
    ImGui::RadioButton("Flat", &lock, static_cast<int>(ViewLock::Flat));
    if (ImGui::BeginItemTooltip()) {
        ImGui::Text("Auto shows the flat map once less than %.0f deg of latitude fits the window height", PlanetCamera::SWITCH_LAT_SPAN_DEG);
        ImGui::EndTooltip();
    }
    if (lock != static_cast<int>(camera.lock)) camera.setLock(static_cast<ViewLock>(lock), viewport, maxZoom());

    const char* projections[] = { "Mercator", "Equirectangular" };
    int projection = static_cast<int>(camera.projection) - static_cast<int>(Projection::Mercator);
    if (ImGui::Combo("Projection", &projection, projections, IM_ARRAYSIZE(projections))) {
        // Keeps the zoom, so the centre of a flat view stays the same size on screen
        camera.projection = static_cast<Projection>(projection + static_cast<int>(Projection::Mercator));
        camera.clampTo(viewport, maxZoom());
    }
    if (ImGui::Checkbox("Wrap around", &camera.wrap)) camera.clampTo(viewport, maxZoom());
    if (ImGui::BeginItemTooltip()) {
        ImGui::Text("Off: the flat map is one map from 180W to 180E that stops at its edges");
        ImGui::EndTooltip();
    }

    ImGui::Separator();
    ImGui::Text("%s, %.0f deg of latitude in view", camera.mode == ViewMode::Globe ? "Globe" : "Flat map", camera.visibleLatSpan(viewport));
    ImGui::Text("Centre %.2f, %.2f  zoom %.0f px/rad (%.1f px per cell)", camera.lat, camera.lon, camera.zoom, camera.zoom * cellSpacing);
    if (hoveredCell >= 0 && hoveredCell < static_cast<int>(planet.cells.size())) {
        const Cell& cell = planet.cells[hoveredCell];
        ImGui::Text("Cell %d at %.2f, %.2f, %d sides", hoveredCell, planet.cellLat[hoveredCell], planet.cellLon[hoveredCell], static_cast<int>(cell.vertex.size()));
        ImGui::Text("Height %.3f (rise %.3f), %s%s, %d cells from the %s", cell.height, cell.rise,
            cell.oceanBool ? "ocean" : "land", cell.coastBool ? ", coast" : "",
            cell.distToOcean == std::numeric_limits<int>::max() ? -1 : cell.distToOcean, cell.oceanBool ? "land" : "ocean");
        if (cell.continent >= 0 && cell.continent < static_cast<int>(globals.continents.size())) {
            const Continent& plate = globals.continents[cell.continent];
            const char* type = plate.plateType == PlateType::Oceanic ? "oceanic" : plate.plateType == PlateType::Continental ? "continental" : "mixed";
            const glm::vec3 velocity = plate.velocityAt(planet.points[hoveredCell]);
            ImGui::Text("Plate %d (%s), moving %.2f toward %.0f deg%s", cell.continent, type,
                glm::length(velocity), tangentToBearing(planet.points[hoveredCell], velocity), cell.volcanicActivity ? ", volcanic" : "");
        }
        ImGui::Text("Temperature %.1f (sd %.1f), precipitation %.1f, humidity %.2f", cell.temp, cell.tempVariance, cell.percepitation, cell.humidity);
        ImGui::Text("Wind %.2f blowing toward %.0f deg", cell.windStr, cell.windDir);
        const bool hasBiome = cell.biome >= 0 && cell.biome < static_cast<int>(globals.biomes.size());
        ImGui::Text("Biome %s%s%s%s", hasBiome ? globals.biomes[cell.biome].name.c_str() : "none",
            cell.riverBool ? ", river" : "", cell.lakeBool ? ", lake" : "", cell.snowBool ? ", snow" : "");
        std::string neighbours;
        for (int nb : cell.neighbors) neighbours += std::to_string(nb) + " ";
        ImGui::TextWrapped("Neighbours: %s", neighbours.c_str());
    }
    else {
        ImGui::Text("No cell under the cursor");
    }
    ImGui::End();
}