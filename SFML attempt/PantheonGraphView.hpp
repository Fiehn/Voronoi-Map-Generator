#pragma once
#include "ReligionManager.hpp"
#include "flecs.h"
#include "HistoryData.hpp"
#include "imgui.h"
#include <map>
#include <cmath>
#include <string>

// Helper structure for graph node positioning
struct GraphNode {
    ImVec2 position;
    ImVec2 velocity;
    float radius = 30.0f;
    flecs::entity deity;
    bool isDragging = false;
};

// Map Flecs relationship entities/types to colors
static ImU32 GetRelationshipColor(const char* rel_name) {
    std::string name(rel_name);
    if (name == "ParentOf")      return IM_COL32(100, 200, 100, 255); // Green
    if (name == "SpouseOf")      return IM_COL32(255, 100, 150, 255); // Pink
    if (name == "ServantOf")     return IM_COL32(150, 150, 200, 255); // Purple
    if (name == "EnemyOf")       return IM_COL32(255, 50, 50, 255);   // Red
    if (name == "SiblingOf")     return IM_COL32(100, 150, 255, 255); // Blue
    if (name == "ChildOf")       return IM_COL32(200, 200, 100, 255); // Yellow
    if (name == "SplitFromDeity")return IM_COL32(200, 150, 100, 255); // Orange
    if (name == "MutilatorOf")   return IM_COL32(100, 0, 0, 255);     // Maroon
    return IM_COL32(128, 128, 128, 255); // Gray
}

class PantheonGraphView {
private:
    std::map<uint64_t, GraphNode> nodes; // flecs entity ID -> node
    flecs::world* world = nullptr;
    flecs::entity currentReligion;

    // Graph physics parameters
    float repulsionStrength = 5000.0f;
    float attractionStrength = 0.01f;
    float damping = 0.8f;
    float centeringForce = 0.001f;

    // UI state
    flecs::entity hoveredDeity;
    flecs::entity selectedDeity;
    ImVec2 graphCenter;
    float graphScale = 1.0f;

    // Layout
    bool autoLayout = true;
    int layoutIterationsPerFrame = 5;

    // Helper to draw directed arrows
    void DrawArrow(ImDrawList* draw_list, ImVec2 from, ImVec2 to, float node_radius, ImU32 color) {
        draw_list->AddLine(from, to, color, 2.0f);
        float arrow_size = 10.0f;
        ImVec2 dir = ImVec2(to.x - from.x, to.y - from.y);
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.1f) {
            dir.x /= len; dir.y /= len;
            ImVec2 arrow_pos = ImVec2(to.x - dir.x * (node_radius + 5.0f), to.y - dir.y * (node_radius + 5.0f));
            ImVec2 perp = ImVec2(-dir.y, dir.x);
            ImVec2 p1 = ImVec2(arrow_pos.x - dir.x * arrow_size + perp.x * arrow_size * 0.5f,
                arrow_pos.y - dir.y * arrow_size + perp.y * arrow_size * 0.5f);
            ImVec2 p2 = ImVec2(arrow_pos.x - dir.x * arrow_size - perp.x * arrow_size * 0.5f,
                arrow_pos.y - dir.y * arrow_size - perp.y * arrow_size * 0.5f);
            draw_list->AddTriangleFilled(arrow_pos, p1, p2, color);
        }
    }

public:
    PantheonGraphView(flecs::world* w)
        : world(w), currentReligion(w->entity(0)), hoveredDeity(w->entity(0)), selectedDeity(w->entity(0)) {
    }

    void SetReligion(flecs::entity rel) {
        if (currentReligion != rel) {
            currentReligion = rel;
            InitializeGraph();
        }
    }

    void InitializeGraph() {
        nodes.clear();
        selectedDeity = world->entity(0);
        hoveredDeity = world->entity(0);

        if (!currentReligion || !currentReligion.is_alive()) return;

        std::vector<flecs::entity> deities;

        world->query_builder<>()
            .with<Deity>()
            .with<WorshippedBy>(currentReligion)
            .without<Extinct>()
            .build()
            .each([&](flecs::entity d) { deities.push_back(d); });

        const float radius = 200.0f;
        for (size_t i = 0; i < deities.size(); i++) {
            GraphNode node;
            float angle = (float)i / deities.size() * 2.0f * 3.14159f;
            node.position = ImVec2(std::cos(angle) * radius, std::sin(angle) * radius);
            node.velocity = ImVec2(0, 0);
            node.deity = deities[i];
            nodes[deities[i].id()] = node;
        }
    }

    void UpdatePhysics() {
        if (!autoLayout) return;

        for (int iter = 0; iter < layoutIterationsPerFrame; iter++) {
            for (auto& [id1, node1] : nodes) {
                ImVec2 force(0, 0);

                // Repulsion
                for (auto& [id2, node2] : nodes) {
                    if (id1 == id2) continue;
                    ImVec2 delta = ImVec2(node1.position.x - node2.position.x, node1.position.y - node2.position.y);
                    float dist = std::max(1.0f, std::sqrt(delta.x * delta.x + delta.y * delta.y));
                    float repulsion = repulsionStrength / (dist * dist);
                    force.x += (delta.x / dist) * repulsion;
                    force.y += (delta.y / dist) * repulsion;
                }

                // Attraction (Edges)
                auto apply_attraction = [&](flecs::entity target) {
                    auto it = nodes.find(target.id());
                    if (it != nodes.end()) {
                        ImVec2 delta = ImVec2(it->second.position.x - node1.position.x, it->second.position.y - node1.position.y);
                        force.x += delta.x * attractionStrength;
                        force.y += delta.y * attractionStrength;
                    }
                    };

                // Apply spring forces for typed relationships
                node1.deity.each<SpouseOf>(apply_attraction);
                node1.deity.each<SiblingOf>(apply_attraction);
                node1.deity.each<ParentOf>(apply_attraction);
                node1.deity.each<EnemyOf>(apply_attraction);
                node1.deity.each<SplitFromDeity>(apply_attraction);

                // Centering force
                force.x -= node1.position.x * centeringForce;
                force.y -= node1.position.y * centeringForce;

                // Apply force
                node1.velocity.x += force.x;
                node1.velocity.y += force.y;
                node1.velocity.x *= damping;
                node1.velocity.y *= damping;
            }

            for (auto& [id, node] : nodes) {
                if (!node.isDragging) {
                    node.position.x += node.velocity.x;
                    node.position.y += node.velocity.y;
                }
            }
        }
    }

    void Draw() {
        ImGui::Begin("Pantheon Graph Viewer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("Auto Layout", &autoLayout);
        if (autoLayout) {
            ImGui::SameLine();
            ImGui::SliderInt("Iterations", &layoutIterationsPerFrame, 1, 20);
        }
        ImGui::SliderFloat("Zoom", &graphScale, 0.1f, 3.0f);
        if (ImGui::Button("Refresh Nodes")) InitializeGraph();

        ImGui::Separator();

        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        if (canvas_size.x < 400.0f) canvas_size.x = 800.0f;
        if (canvas_size.y < 300.0f) canvas_size.y = 600.0f;

        // Reserve place in the layout
        ImGui::InvisibleButton("canvas", canvas_size);

        graphCenter = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 30, 255));

        DrawRelationships(draw_list);
        DrawNodes(draw_list);
        HandleInput(canvas_pos, canvas_size);

        if (autoLayout) UpdatePhysics();

        ImGui::End();

        if (selectedDeity && selectedDeity.is_alive()) {
            DrawDeityDetails();
        }
    }

    void DrawRelationships(ImDrawList* draw_list) {
        auto draw_edge = [&](flecs::entity source, flecs::entity target, const char* rel_name) {
            auto it_source = nodes.find(source.id());
            auto it_target = nodes.find(target.id());
            if (it_source != nodes.end() && it_target != nodes.end()) {
                ImVec2 p1 = WorldToScreen(it_source->second.position);
                ImVec2 p2 = WorldToScreen(it_target->second.position);
                ImU32 col = GetRelationshipColor(rel_name);
                DrawArrow(draw_list, p1, p2, it_target->second.radius * graphScale, col);
            }
            };

        // Explicitly typed iterators to satisfy the compiler
        for (auto& [id, node] : nodes) {
            node.deity.each<SpouseOf>([&](flecs::entity target) { draw_edge(node.deity, target, "SpouseOf"); });
            node.deity.each<SiblingOf>([&](flecs::entity target) { draw_edge(node.deity, target, "SiblingOf"); });
            node.deity.each<EnemyOf>([&](flecs::entity target) { draw_edge(node.deity, target, "EnemyOf"); });
            node.deity.each<ParentOf>([&](flecs::entity target) { draw_edge(node.deity, target, "ParentOf"); });
            node.deity.each<SplitFromDeity>([&](flecs::entity target) { draw_edge(node.deity, target, "SplitFromDeity"); });
        }
    }

    void DrawNodes(ImDrawList* draw_list) {
        for (auto& [id, node] : nodes) {
            ImVec2 screen_pos = WorldToScreen(node.position);
            float screen_radius = node.radius * graphScale;

            ImU32 node_color = IM_COL32(70, 130, 180, 255);
            if (selectedDeity == node.deity) node_color = IM_COL32(255, 215, 0, 255);
            else if (hoveredDeity == node.deity) node_color = IM_COL32(100, 200, 255, 255);

            draw_list->AddCircleFilled(screen_pos, screen_radius, node_color);
            draw_list->AddCircle(screen_pos, screen_radius, IM_COL32(255, 255, 255, 255), 0, 2.0f);

            const char* name = node.deity.name().c_str();
            if (name) {
                ImVec2 text_size = ImGui::CalcTextSize(name);
                draw_list->AddText(ImVec2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f),
                    IM_COL32(255, 255, 255, 255), name);
            }
        }
    }

    void HandleInput(ImVec2 canvas_pos, ImVec2 canvas_size) {
        ImGuiIO& io = ImGui::GetIO();
        hoveredDeity = world->entity(0);
        ImVec2 mouse_pos = io.MousePos;

        if (mouse_pos.x >= canvas_pos.x && mouse_pos.x <= canvas_pos.x + canvas_size.x &&
            mouse_pos.y >= canvas_pos.y && mouse_pos.y <= canvas_pos.y + canvas_size.y) {

            for (auto& [id, node] : nodes) {
                ImVec2 screen_pos = WorldToScreen(node.position);
                float dist = std::sqrt(std::pow(mouse_pos.x - screen_pos.x, 2) + std::pow(mouse_pos.y - screen_pos.y, 2));

                if (dist <= node.radius * graphScale) {
                    hoveredDeity = node.deity;
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        selectedDeity = hoveredDeity;
                        node.isDragging = true;
                    }
                    break;
                }
            }
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            for (auto& [id, node] : nodes) {
                if (node.isDragging) {
                    node.position.x += io.MouseDelta.x / graphScale;
                    node.position.y += io.MouseDelta.y / graphScale;
                    node.velocity = ImVec2(0, 0);
                }
            }
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            for (auto& [id, node] : nodes) node.isDragging = false;
        }
    }

    void DrawDeityDetails() {
        ImGui::Begin("Deity Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Name: %s", selectedDeity.name().c_str());

        selectedDeity.each<Archetype>([&](flecs::entity arch) {
            ImGui::Text("Archetype: %s", arch.name().c_str());
            });

        ImGui::Separator();
        ImGui::Text("Domains:");

        selectedDeity.each<GodOf>([&](flecs::entity domain) {
            ImGui::BulletText("%s", domain.name().c_str());
            });

        ImGui::Separator();
        ImGui::Text("Relationships:");

        auto print_rel = [&](flecs::entity target, const char* label, const char* rel_name) {
            ImGui::PushStyleColor(ImGuiCol_Text, GetRelationshipColor(rel_name));
            ImGui::BulletText("%s of %s", label, target.name().c_str());
            ImGui::PopStyleColor();
            };

        // Explicitly typed iterators replace the dynamic lookups
        selectedDeity.each<SpouseOf>([&](flecs::entity target) { print_rel(target, "Spouse", "SpouseOf"); });
        selectedDeity.each<SiblingOf>([&](flecs::entity target) { print_rel(target, "Sibling", "SiblingOf"); });
        selectedDeity.each<EnemyOf>([&](flecs::entity target) { print_rel(target, "Enemy", "EnemyOf"); });
        selectedDeity.each<ParentOf>([&](flecs::entity target) { print_rel(target, "Parent", "ParentOf"); });
        selectedDeity.each<SplitFromDeity>([&](flecs::entity target) { print_rel(target, "Split From", "SplitFromDeity"); });

        ImGui::End();
    }

    ImVec2 WorldToScreen(ImVec2 world_pos) {
        return ImVec2(graphCenter.x + world_pos.x * graphScale, graphCenter.y + world_pos.y * graphScale);
    }
};

void DrawReligionControls(flecs::world& world, ReligionManager& relManager, History& history, Cell& testCell, vor::Voronoi& map) {
    static flecs::entity activeReligion = world.entity(0);
    static PantheonGraphView graphView(&world);
    static uint32_t currentTick = 0;

    ImGui::Begin("Myth Generator");

    if (ImGui::Button("Generate Proto-Religion", ImVec2(200, 40))) {
        currentTick = 0;

        // Clean up previous test religion safely
        if (activeReligion && activeReligion.is_alive()) {
            activeReligion.destruct();
        }

        // Generate religion
        relManager.create_proto_religion(world, history, testCell);

        // Reverted to query_builder<Religion> 
        world.query_builder<>()
            .with<Religion>()
            .without<Extinct>()
            .build()
            .each([&](flecs::entity rel) { activeReligion = rel; });

        // Bind it to the viewer
        graphView.SetReligion(activeReligion);
    }

    if (activeReligion && activeReligion.is_alive()) {
        ImGui::SameLine();
        if (ImGui::Button("Step Myth Phase (1 Tick)", ImVec2(200, 40))) {
            currentTick++;
            relManager.myth_phase_evolution(world, history, activeReligion, currentTick, map);

            // Re-initialize graph to catch newly created/destroyed deities
            graphView.InitializeGraph();
        }
    }

    ImGui::End();

    // Draw the Graph Viewer
    if (activeReligion && activeReligion.is_alive()) {
        graphView.Draw();
    }

    // === History Log Window ===
    ImGui::Begin("Chronicle of History");
    if (ImGui::Button("Clear History")) history.clear();
    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& ev : history.events) {
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
        if (ev.type == EventType::DeityCreation) color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
        if (ev.type == EventType::DeityInteraction) color = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, color);

        std::string causal_link = ev.caused_by_event_id ? " [Caused by Event #" + std::to_string(*ev.caused_by_event_id) + "]" : "";

        ImGui::TextWrapped("[Tick %d] Event #%llu: %s %s %s. (Reason: %s)%s",
            ev.tick,
            ev.event_id,
            ev.snapshot.subject_name.c_str(),
            ev.snapshot.location_name.c_str(),
            ev.snapshot.object_name.c_str(),
            ev.snapshot.reason.c_str(),
            causal_link.c_str());

        ImGui::PopStyleColor();
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}