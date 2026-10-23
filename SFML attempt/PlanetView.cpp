#include "PlanetView.hpp"
#include "Logger.h"
#include "imgui.h"
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
    recolor(planet);
}

void PlanetView::recolor(const vor::SphereVoronoi& planet)
{
    renderer.setCellColors(debugCellColors(planet, colorMode));
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
        && camera.globePick(glm::vec2(static_cast<float>(mouse.x), static_cast<float>(mouse.y)), viewport, hit)) {
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

void PlanetView::ui(vor::SphereVoronoi& planet, sf::RenderWindow& window)
{
    ImGui::Begin("Globe (preview)");
    ImGui::TextWrapped("Sphere cells with debug colours. The simulation still runs on the flat map until it is ported to the sphere.");

    ImGui::InputInt("Cells", &previewCells, 1000, 10000);
    previewCells = std::max(100, std::min(previewCells, 1000000));
    ImGui::SliderFloat("Jitter", &previewJitter, 0.f, 1.f);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("How far each point may move from its spiral position, in cell spacings"); ImGui::EndTooltip(); }

    if (ImGui::Button("Regenerate") || !hasPlanet()) {
        planet.fillMap(previewCells, previewJitter);
        const vor::SphereValidation check = vor::validate(planet);
        if (check.ok) { LOG_INFO(Map, "Sphere Voronoi valid:", check.report); }
        else { LOG_ERROR(Map, "Sphere Voronoi failed validation:", check.report); }
        if (window.setActive(true)) rebuild(planet);
    }

    const int oldMode = colorMode;
    ImGui::RadioButton("Cells", &colorMode, 0); ImGui::SameLine();
    ImGui::RadioButton("Lat/Lon grid", &colorMode, 1);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("White: prime meridian, red: 90E, warm: north, cool: south"); ImGui::EndTooltip(); }
    if (colorMode != oldMode && window.setActive(true)) recolor(planet);

    ImGui::Separator();
    ImGui::Text("Centre %.2f, %.2f  zoom %.0f px/rad (%.1f px per cell)", camera.lat, camera.lon, camera.zoom, camera.zoom * cellSpacing);
    if (hoveredCell >= 0 && hoveredCell < static_cast<int>(planet.cells.size())) {
        const Cell& cell = planet.cells[hoveredCell];
        ImGui::Text("Cell %d at %.2f, %.2f, %d sides", hoveredCell, planet.cellLat[hoveredCell], planet.cellLon[hoveredCell], static_cast<int>(cell.vertex.size()));
        std::string neighbours;
        for (int nb : cell.neighbors) neighbours += std::to_string(nb) + " ";
        ImGui::TextWrapped("Neighbours: %s", neighbours.c_str());
    }
    else {
        ImGui::Text("No cell under the cursor");
    }
    ImGui::End();
}
