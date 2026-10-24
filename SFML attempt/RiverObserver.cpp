#include "RiverObserver.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include "imgui.h"

namespace {

    constexpr float HEIGHT_UNIT_M = 10000.f; // Height 1 is 10 km, as in the hydrology step
    constexpr float TABLE_HEIGHT = 240.f;

    // Column ids, shared by the sort and the cells
    enum RiverColumn : ImGuiID { RC_ID = 1, RC_LENGTH, RC_DISCHARGE, RC_WIDTH, RC_DEPTH, RC_SPEED, RC_SOURCE, RC_CELLS, RC_TRIBS, RC_MOUTH };
    enum LakeColumn : ImGuiID { LC_ID = 1, LC_AREA, LC_DEPTH, LC_INFLOW, LC_EVAP, LC_OUTFLOW, LC_CELLS, LC_RIVERS, LC_STATUS };

    struct RiverRow {
        int id = 0;
        float lengthKm = 0.f, mouthQ = 0.f;
        float width = 0.f, depth = 0.f, speed = 0.f; // The largest along the river
        float sourceM = 0.f;                         // Source above sea level
        int cells = 0, tributaries = 0;
        int mouthType = 0;
        int mouthLake = -1;
        int parent = -1;
    };

    struct LakeRow {
        int id = 0;
        float areaKm2 = 0.f, depthM = 0.f, inflow = 0.f, evaporation = 0.f, outflow = 0.f;
        int cells = 0, riversIn = 0;
        bool draining = false;
    };

    const char* mouthName(RiverMouth type)
    {
        switch (type) {
        case RiverMouth::Ocean: return "Ocean";
        case RiverMouth::Lake: return "Lake";
        case RiverMouth::River: return "River";
        default: return "Closed";
        }
    }

    std::string mouthLabel(const RiverRow& row)
    {
        const RiverMouth type = static_cast<RiverMouth>(row.mouthType);
        if (type == RiverMouth::Lake) return "Lake " + std::to_string(row.mouthLake);
        if (type == RiverMouth::River) return "River " + std::to_string(row.parent);
        return mouthName(type);
    }

    double riverKey(const RiverRow& r, ImGuiID column)
    {
        switch (column) {
        case RC_LENGTH: return r.lengthKm;
        case RC_DISCHARGE: return r.mouthQ;
        case RC_WIDTH: return r.width;
        case RC_DEPTH: return r.depth;
        case RC_SPEED: return r.speed;
        case RC_SOURCE: return r.sourceM;
        case RC_CELLS: return r.cells;
        case RC_TRIBS: return r.tributaries;
        case RC_MOUTH: return r.mouthType;
        default: return r.id;
        }
    }

    double lakeKey(const LakeRow& l, ImGuiID column)
    {
        switch (column) {
        case LC_AREA: return l.areaKm2;
        case LC_DEPTH: return l.depthM;
        case LC_INFLOW: return l.inflow;
        case LC_EVAP: return l.evaporation;
        case LC_OUTFLOW: return l.outflow;
        case LC_CELLS: return l.cells;
        case LC_RIVERS: return l.riversIn;
        case LC_STATUS: return l.draining ? 1 : 0;
        default: return l.id;
        }
    }

    template <typename Row, typename Key>
    void sortRows(std::vector<Row>& rows, ImGuiTableSortSpecs* specs, Key key)
    {
        if (!specs || specs->SpecsCount == 0) return;
        std::sort(rows.begin(), rows.end(), [&](const Row& a, const Row& b) {
            for (int i = 0; i < specs->SpecsCount; i++) {
                const ImGuiTableColumnSortSpecs& spec = specs->Specs[i];
                const double va = key(a, spec.ColumnUserID), vb = key(b, spec.ColumnUserID);
                if (va != vb) return spec.SortDirection == ImGuiSortDirection_Ascending ? va < vb : va > vb;
            }
            return a.id < b.id;
            });
    }

    // Whole numbers without decimals, small ones with a few
    void number(float value)
    {
        if (std::abs(value) >= 100.f) ImGui::Text("%.0f", value);
        else if (std::abs(value) >= 10.f) ImGui::Text("%.1f", value);
        else ImGui::Text("%.2f", value);
    }

    // A button that doesn't clash with the same label elsewhere in the window
    bool link(const std::string& text, int id)
    {
        ImGui::PushID(id);
        const bool pressed = ImGui::SmallButton(text.c_str());
        ImGui::PopID();
        return pressed;
    }
}

void RiverObserver::reset()
{
    selectedRiver = selectedLake = -1;
    highlightCells.clear();
}

void RiverObserver::selectRiver(int id, const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window)
{
    if (id < 0 || id >= static_cast<int>(globals.rivers.size())) return;
    selectedRiver = id;
    selectedLake = -1;
    highlightCells = globals.rivers[id].getPathCells();
    view.focusOn(planet, highlightCells, window);
}

void RiverObserver::selectLake(int id, const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window)
{
    if (id < 0 || id >= static_cast<int>(globals.waterBodies.size())) return;
    selectedLake = id;
    selectedRiver = -1;
    highlightCells.clear();
    for (int cell : globals.waterBodies[id].cells) highlightCells.push_back(static_cast<std::size_t>(cell));
    view.focusOn(planet, highlightCells, window);
}

void RiverObserver::ui(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window)
{
    if (!open) return;

    // The selection belongs to one world
    if (selectedRiver >= static_cast<int>(globals.rivers.size()) || selectedLake >= static_cast<int>(globals.waterBodies.size())) reset();

    ImGui::SetNextWindowSize(ImVec2(720.f, 620.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("River Observer", &open)) {
        ImGui::End();
        return;
    }

    float totalKm = 0.f;
    int mainRivers = 0;
    for (const River& river : globals.rivers) {
        if (river.parentRiver < 0) { totalKm += river.lengthKm; mainRivers++; }
    }
    int draining = 0;
    float lakeArea = 0.f;
    for (const WaterBody& lake : globals.waterBodies) { draining += lake.full && lake.outflow > 0.f; lakeArea += lake.areaKm2; }
    ImGui::Text("%d rivers (%d main, %.0f km), %d lakes (%d draining, %.0f km2)", static_cast<int>(globals.rivers.size()), mainRivers, totalKm,
        static_cast<int>(globals.waterBodies.size()), draining, lakeArea);

    if (ImGui::BeginTabBar("ObserverTabs")) {
        const bool showRivers = ImGui::BeginTabItem("Rivers", nullptr, pendingTab == 0 ? ImGuiTabItemFlags_SetSelected : 0);
        if (showRivers) { riversTab(planet, globals, view, window); ImGui::EndTabItem(); }
        const bool showLakes = ImGui::BeginTabItem("Lakes", nullptr, pendingTab == 1 ? ImGuiTabItemFlags_SetSelected : 0);
        if (showLakes) { lakesTab(planet, globals, view, window); ImGui::EndTabItem(); }
        pendingTab = -1;
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void RiverObserver::riversTab(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window)
{
    ImGui::Checkbox("Main rivers only", &mainRiversOnly);
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Hide the tributaries"); ImGui::EndTooltip(); }

    std::vector<RiverRow> rows;
    rows.reserve(globals.rivers.size());
    for (const River& river : globals.rivers) {
        if (mainRiversOnly && river.parentRiver >= 0) continue;
        RiverRow row;
        row.id = river.id;
        row.lengthKm = river.lengthKm;
        row.mouthQ = river.mouthDischarge;
        row.cells = static_cast<int>(river.getPathCells().size());
        row.tributaries = static_cast<int>(river.tributaries.size());
        row.mouthType = static_cast<int>(river.mouthType);
        row.mouthLake = river.mouthLake;
        row.parent = river.parentRiver;
        for (std::size_t c : river.getPathCells()) {
            const Cell& cell = planet.cells[c];
            if (cell.riverId != river.id) continue; // The cell a tributary joins is the parent's
            row.width = std::max(row.width, cell.riverWidth);
            row.depth = std::max(row.depth, cell.riverDepth);
            row.speed = std::max(row.speed, cell.riverSpeed);
        }
        if (!river.getPathCells().empty()) row.sourceM = (planet.cells[river.getPathCells().front()].height - globals.seaLevel) * HEIGHT_UNIT_M;
        rows.push_back(row);
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("Rivers", 10, flags, ImVec2(0.f, TABLE_HEIGHT))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_PreferSortAscending, 0.f, RC_ID);
        ImGui::TableSetupColumn("Length km", ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_LENGTH);
        ImGui::TableSetupColumn("Mouth m3/s", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_DISCHARGE);
        ImGui::TableSetupColumn("Width m", ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_WIDTH);
        ImGui::TableSetupColumn("Depth m", ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_DEPTH);
        ImGui::TableSetupColumn("Speed m/s", ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_SPEED);
        ImGui::TableSetupColumn("Source m", ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_SOURCE);
        ImGui::TableSetupColumn("Cells", ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_CELLS);
        ImGui::TableSetupColumn("Tribs", ImGuiTableColumnFlags_PreferSortDescending, 0.f, RC_TRIBS);
        ImGui::TableSetupColumn("Mouth", 0, 0.f, RC_MOUTH);
        ImGui::TableHeadersRow();

        sortRows(rows, ImGui::TableGetSortSpecs(), riverKey);

        for (const RiverRow& row : rows) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(row.id);
            if (ImGui::Selectable(std::to_string(row.id).c_str(), selectedRiver == row.id, ImGuiSelectableFlags_SpanAllColumns)) {
                selectRiver(row.id, planet, globals, view, window);
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(1); number(row.lengthKm);
            ImGui::TableSetColumnIndex(2); number(row.mouthQ);
            ImGui::TableSetColumnIndex(3); number(row.width);
            ImGui::TableSetColumnIndex(4); number(row.depth);
            ImGui::TableSetColumnIndex(5); number(row.speed);
            ImGui::TableSetColumnIndex(6); ImGui::Text("%.0f", row.sourceM);
            ImGui::TableSetColumnIndex(7); ImGui::Text("%d", row.cells);
            ImGui::TableSetColumnIndex(8); ImGui::Text("%d", row.tributaries);
            ImGui::TableSetColumnIndex(9); ImGui::TextUnformatted(mouthLabel(row).c_str());
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    if (selectedRiver < 0 || selectedRiver >= static_cast<int>(globals.rivers.size())) {
        ImGui::TextDisabled("Click a river to select it. The map outlines its cells and moves to it");
        return;
    }

    const River& river = globals.rivers[selectedRiver];
    const std::vector<std::size_t>& path = river.getPathCells();
    ImGui::Text("River %d: %.0f km long, %d cells, %.0f m3/s at the mouth", river.id, river.lengthKm, static_cast<int>(path.size()), river.mouthDischarge);
    if (!path.empty()) {
        const Cell& source = planet.cells[path.front()];
        const Cell& last = planet.cells[path.back()];
        ImGui::Text("Source: cell %d, %.0f m above sea level, %.0f m3/s", static_cast<int>(path.front()), (source.height - globals.seaLevel) * HEIGHT_UNIT_M, source.discharge);
        ImGui::Text("Ends at cell %d: %.0f m wide, %.1f m deep, %.2f m/s", static_cast<int>(path.back()), last.riverWidth, last.riverDepth, last.riverSpeed);
    }

    ImGui::Text("Mouth: %s", mouthName(river.mouthType));
    if (river.mouthType == RiverMouth::Lake && river.mouthLake >= 0) {
        ImGui::SameLine();
        if (link("Lake " + std::to_string(river.mouthLake), 1000)) { selectLake(river.mouthLake, planet, globals, view, window); pendingTab = 1; }
    }
    if (river.parentRiver >= 0) {
        ImGui::SameLine();
        ImGui::Text("joins");
        ImGui::SameLine();
        if (link("River " + std::to_string(river.parentRiver), 1001)) selectRiver(river.parentRiver, planet, globals, view, window);
    }
    for (std::size_t b = 0; b < globals.waterBodies.size(); b++) {
        if (globals.waterBodies[b].outflowRiver != river.id) continue;
        ImGui::Text("Starts at the outlet of");
        ImGui::SameLine();
        if (link("Lake " + std::to_string(b), 1002)) { selectLake(static_cast<int>(b), planet, globals, view, window); pendingTab = 1; }
    }
    if (!river.tributaries.empty()) {
        ImGui::Text("Tributaries:");
        for (int tributary : river.tributaries) {
            ImGui::SameLine();
            if (link(std::to_string(tributary), 2000 + tributary)) { selectRiver(tributary, planet, globals, view, window); break; }
        }
    }
}

void RiverObserver::lakesTab(const vor::SphereVoronoi& planet, const GlobalWorldObjects& globals, PlanetView& view, const sf::RenderWindow& window)
{
    const char* filters[] = { "All", "Draining", "Closed basins" };
    ImGui::SetNextItemWidth(160.f);
    ImGui::Combo("Show", &lakeFilter, filters, IM_ARRAYSIZE(filters));
    if (ImGui::BeginItemTooltip()) { ImGui::Text("Draining lakes spill over an outlet, closed basins lose all their water to evaporation"); ImGui::EndTooltip(); }

    std::vector<LakeRow> rows;
    rows.reserve(globals.waterBodies.size());
    for (std::size_t i = 0; i < globals.waterBodies.size(); i++) {
        const WaterBody& lake = globals.waterBodies[i];
        const bool draining = lake.full && lake.outflow > 0.f;
        if ((lakeFilter == 1 && !draining) || (lakeFilter == 2 && draining)) continue;
        LakeRow row;
        row.id = static_cast<int>(i);
        row.areaKm2 = lake.areaKm2;
        row.depthM = lake.depthM;
        row.inflow = lake.inflow;
        row.evaporation = lake.evaporation;
        row.outflow = lake.outflow;
        row.cells = static_cast<int>(lake.cells.size());
        row.riversIn = static_cast<int>(lake.inflowRivers.size());
        row.draining = draining;
        rows.push_back(row);
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("Lakes", 9, flags, ImVec2(0.f, TABLE_HEIGHT))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_PreferSortAscending, 0.f, LC_ID);
        ImGui::TableSetupColumn("Area km2", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 0.f, LC_AREA);
        ImGui::TableSetupColumn("Depth m", ImGuiTableColumnFlags_PreferSortDescending, 0.f, LC_DEPTH);
        ImGui::TableSetupColumn("In m3/s", ImGuiTableColumnFlags_PreferSortDescending, 0.f, LC_INFLOW);
        ImGui::TableSetupColumn("Evap m3/s", ImGuiTableColumnFlags_PreferSortDescending, 0.f, LC_EVAP);
        ImGui::TableSetupColumn("Out m3/s", ImGuiTableColumnFlags_PreferSortDescending, 0.f, LC_OUTFLOW);
        ImGui::TableSetupColumn("Cells", ImGuiTableColumnFlags_PreferSortDescending, 0.f, LC_CELLS);
        ImGui::TableSetupColumn("Rivers in", ImGuiTableColumnFlags_PreferSortDescending, 0.f, LC_RIVERS);
        ImGui::TableSetupColumn("Status", 0, 0.f, LC_STATUS);
        ImGui::TableHeadersRow();

        sortRows(rows, ImGui::TableGetSortSpecs(), lakeKey);

        for (const LakeRow& row : rows) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(row.id);
            if (ImGui::Selectable(std::to_string(row.id).c_str(), selectedLake == row.id, ImGuiSelectableFlags_SpanAllColumns)) {
                selectLake(row.id, planet, globals, view, window);
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(1); number(row.areaKm2);
            ImGui::TableSetColumnIndex(2); number(row.depthM);
            ImGui::TableSetColumnIndex(3); number(row.inflow);
            ImGui::TableSetColumnIndex(4); number(row.evaporation);
            ImGui::TableSetColumnIndex(5); number(row.outflow);
            ImGui::TableSetColumnIndex(6); ImGui::Text("%d", row.cells);
            ImGui::TableSetColumnIndex(7); ImGui::Text("%d", row.riversIn);
            ImGui::TableSetColumnIndex(8); ImGui::TextUnformatted(row.draining ? "Draining" : "Closed");
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    if (selectedLake < 0 || selectedLake >= static_cast<int>(globals.waterBodies.size())) {
        ImGui::TextDisabled("Click a lake to select it. The map outlines its cells and moves to it");
        return;
    }

    const WaterBody& lake = globals.waterBodies[selectedLake];
    ImGui::Text("Lake %d: %.0f km2 over %d cells, %.0f m deep, surface at %.0f m above sea level", selectedLake, lake.areaKm2,
        static_cast<int>(lake.cells.size()), lake.depthM, (lake.level - globals.seaLevel) * HEIGHT_UNIT_M);
    ImGui::Text("Receives %.1f m3/s, evaporates %.1f m3/s, %s", lake.inflow, lake.evaporation,
        lake.full && lake.outflow > 0.f ? "the rest leaves over the outlet" : "a closed basin");
    if (lake.outlet >= 0) ImGui::Text("Outlet: cell %d, %.1f m3/s", lake.outlet, lake.outflow);
    if (lake.outflowRiver >= 0) {
        ImGui::Text("Outflow:");
        ImGui::SameLine();
        if (link("River " + std::to_string(lake.outflowRiver), 3000)) { selectRiver(lake.outflowRiver, planet, globals, view, window); pendingTab = 0; }
    }
    if (!lake.inflowRivers.empty()) {
        ImGui::Text("Inflow:");
        for (int river : lake.inflowRivers) {
            ImGui::SameLine();
            if (link("River " + std::to_string(river), 4000 + river)) { selectRiver(river, planet, globals, view, window); pendingTab = 0; break; }
        }
    }
}
