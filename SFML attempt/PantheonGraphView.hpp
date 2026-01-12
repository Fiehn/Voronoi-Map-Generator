#pragma once
#include "ReligionManager.hpp"
#include "ReligionData.hpp"
#include "cell.hpp"
#include "imgui.h"
#include <map>
#include <cmath>

// Helper structure for graph node positioning
struct GraphNode {
    ImVec2 position;
    ImVec2 velocity;
    float radius = 30.0f;
    DeityHandle handle;
    bool isDragging = false;
};

// Color mapping for relationship types
static ImU32 GetRelationshipColor(DeityRelationship rel) {
    switch (rel) {
        case DeityRelationship::ParentOf:     return IM_COL32(100, 200, 100, 255); // Green
        case DeityRelationship::SpouseOf:     return IM_COL32(255, 100, 150, 255); // Pink
        case DeityRelationship::ServantOf:    return IM_COL32(150, 150, 200, 255); // Purple
        case DeityRelationship::EnemyOf:      return IM_COL32(255, 50, 50, 255);   // Red
        case DeityRelationship::SiblingOf:    return IM_COL32(100, 150, 255, 255); // Blue
        case DeityRelationship::SlayerOf:     return IM_COL32(150, 0, 0, 255);     // Dark red
        case DeityRelationship::ProtectorOf:  return IM_COL32(200, 200, 100, 255); // Yellow
        case DeityRelationship::CreatorOf:    return IM_COL32(200, 150, 100, 255); // Orange
        case DeityRelationship::AuthorOf:     return IM_COL32(150, 100, 200, 255); // Lavender
        case DeityRelationship::MutilatorOf:  return IM_COL32(100, 0, 0, 255);     // Maroon
        default:                              return IM_COL32(128, 128, 128, 255); // Gray
    }
}

static const char* GetRelationshipName(DeityRelationship rel) {
    switch (rel) {
        case DeityRelationship::ParentOf:     return "Parent";
        case DeityRelationship::SpouseOf:     return "Spouse";
        case DeityRelationship::ServantOf:    return "Servant";
        case DeityRelationship::EnemyOf:      return "Enemy";
        case DeityRelationship::SiblingOf:    return "Sibling";
        case DeityRelationship::SlayerOf:     return "Slayer";
        case DeityRelationship::ProtectorOf:  return "Protector";
        case DeityRelationship::CreatorOf:    return "Creator";
        case DeityRelationship::AuthorOf:     return "Author";
        case DeityRelationship::MutilatorOf:  return "Mutilator";
        default:                              return "Unknown";
    }
}

class PantheonGraphView {
private:
    std::map<uint16_t, GraphNode> nodes; // deity index -> node
    ReligionManager* religionManager = nullptr;
    ReligionHandle currentReligion;
    
    // Graph physics parameters
    float repulsionStrength = 5000.0f;
    float attractionStrength = 0.01f;
    float damping = 0.8f;
    float centeringForce = 0.001f;
    
    // UI state
    DeityHandle hoveredDeity;
    DeityHandle selectedDeity;
    ImVec2 graphCenter;
    float graphScale = 1.0f;
    
    // Layout
    bool autoLayout = true;
    int layoutIterationsPerFrame = 5;
    
public:
    PantheonGraphView(ReligionManager* manager) : religionManager(manager) {
        hoveredDeity.index = (uint16_t)-1;
        selectedDeity.index = (uint16_t)-1;
    }
    
    void SetReligion(ReligionHandle handle) {
        currentReligion = handle;
        InitializeGraph();
    }
    
    void InitializeGraph() {
        nodes.clear();
        
        if (!religionManager->IsValid(currentReligion)) {
            return;
        }
        
        auto deities = religionManager->GetPantheonDeities(currentReligion);
        
        // Initialize nodes in a circle
        const float radius = 200.0f;
        for (size_t i = 0; i < deities.size(); i++) {
            GraphNode node;
            float angle = (float)i / deities.size() * 2.0f * 3.14159f;
            node.position = ImVec2(
                std::cos(angle) * radius,
                std::sin(angle) * radius
            );
            node.velocity = ImVec2(0, 0);
            node.handle = deities[i];
            nodes[deities[i].index] = node;
        }
    }
    
    void UpdatePhysics() {
        if (!autoLayout) return;
        
        for (int iter = 0; iter < layoutIterationsPerFrame; iter++) {
            // Calculate forces
            for (auto& [id1, node1] : nodes) {
                ImVec2 force(0, 0);
                
                // Repulsion from all other nodes
                for (auto& [id2, node2] : nodes) {
                    if (id1 == id2) continue;
                    
                    ImVec2 delta = ImVec2(
                        node1.position.x - node2.position.x,
                        node1.position.y - node2.position.y
                    );
                    float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                    if (dist < 1.0f) dist = 1.0f;
                    
                    float repulsion = repulsionStrength / (dist * dist);
                    force.x += (delta.x / dist) * repulsion;
                    force.y += (delta.y / dist) * repulsion;
                }
                
                // Attraction along relationships
                auto& deityData = religionManager->GetDeityData();
                uint32_t offset = deityData.relationship_offsets[id1];
                uint32_t count = deityData.relationship_counts[id1];
                
                for (uint32_t i = 0; i < count; i++) {
                    // Get target deity from relationship data
                    // This requires access to deity relationships
                    // We'll add attraction to connected nodes
                }
                
                // Centering force
                force.x -= node1.position.x * centeringForce;
                force.y -= node1.position.y * centeringForce;
                
                // Apply force
                node1.velocity.x += force.x;
                node1.velocity.y += force.y;
                node1.velocity.x *= damping;
                node1.velocity.y *= damping;
            }
            
            // Update positions
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
        
        // Controls
        ImGui::Checkbox("Auto Layout", &autoLayout);
        if (autoLayout) {
            ImGui::SameLine();
            ImGui::SliderInt("Iterations/Frame", &layoutIterationsPerFrame, 1, 20);
        }
        ImGui::SliderFloat("Zoom", &graphScale, 0.1f, 3.0f);
        
        if (ImGui::Button("Reset Layout")) {
            InitializeGraph();
        }
        
        ImGui::Separator();
        
        // Graph canvas
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        if (canvas_size.x < 50.0f) canvas_size.x = 800.0f;
        if (canvas_size.y < 50.0f) canvas_size.y = 600.0f;
        
        graphCenter = ImVec2(
            canvas_pos.x + canvas_size.x * 0.5f,
            canvas_pos.y + canvas_size.y * 0.5f
        );
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_pos, 
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(30, 30, 30, 255));
        draw_list->AddRect(canvas_pos, 
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(100, 100, 100, 255));
        
        // Draw relationships (edges)
        DrawRelationships(draw_list);
        
        // Draw nodes (deities)
        DrawNodes(draw_list);
        
        // Handle input
        HandleInput(canvas_pos, canvas_size);
        
        // Draw legend
        DrawLegend();
        
        // Update physics
        if (autoLayout) {
            UpdatePhysics();
        }
        
        ImGui::End();
        
        // Draw deity details panel
        if (selectedDeity.isValid()) {
            DrawDeityDetails();
        }
    }
    
    void DrawRelationships(ImDrawList* draw_list) {
        if (!religionManager->IsValid(currentReligion)) return;
        
        auto& deityData = religionManager->GetDeityData();
        auto& relData = religionManager->GetDeityRelationshipData();
        
        for (auto& [id, node] : nodes) {
            uint32_t offset = deityData.relationship_offsets[id];
            uint32_t count = deityData.relationship_counts[id];
            
            ImVec2 from_pos = WorldToScreen(node.position);
            
            // Draw each relationship
            for (uint32_t i = 0; i < count; i++) {
                uint32_t rel_idx = offset + i;
                uint16_t target_id = relData.target_deity_ids[rel_idx];
                DeityRelationship rel_type = relData.relationship_types[rel_idx];
                
                // Find the target node
                auto it = nodes.find(target_id);
                if (it == nodes.end()) continue;
                
                ImVec2 to_pos = WorldToScreen(it->second.position);
                ImU32 color = GetRelationshipColor(rel_type);
                
                // Draw directed arrow
                draw_list->AddLine(from_pos, to_pos, color, 2.0f);
                
                // Draw arrowhead
                float arrow_size = 10.0f;
                ImVec2 direction = ImVec2(to_pos.x - from_pos.x, to_pos.y - from_pos.y);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > 0.1f) {
                    direction.x /= length;
                    direction.y /= length;
                    
                    ImVec2 arrow_pos = ImVec2(
                        to_pos.x - direction.x * (nodes[target_id].radius * graphScale + 5.0f),
                        to_pos.y - direction.y * (nodes[target_id].radius * graphScale + 5.0f)
                    );
                    
                    ImVec2 perp = ImVec2(-direction.y, direction.x);
                    ImVec2 p1 = ImVec2(arrow_pos.x - direction.x * arrow_size + perp.x * arrow_size * 0.5f,
                                      arrow_pos.y - direction.y * arrow_size + perp.y * arrow_size * 0.5f);
                    ImVec2 p2 = ImVec2(arrow_pos.x - direction.x * arrow_size - perp.x * arrow_size * 0.5f,
                                      arrow_pos.y - direction.y * arrow_size - perp.y * arrow_size * 0.5f);
                    
                    draw_list->AddTriangleFilled(arrow_pos, p1, p2, color);
                }
            }
        }
    }
    
    void DrawNodes(ImDrawList* draw_list) {
        if (!religionManager->IsValid(currentReligion)) return;
        
        auto& deityData = religionManager->GetDeityData();
        
        for (auto& [id, node] : nodes) {
            ImVec2 screen_pos = WorldToScreen(node.position);
            float screen_radius = node.radius * graphScale;
            
            // Determine color based on state
            ImU32 node_color = IM_COL32(70, 130, 180, 255); // Steel blue
            if (selectedDeity.index == id) {
                node_color = IM_COL32(255, 215, 0, 255); // Gold
            } else if (hoveredDeity.index == id) {
                node_color = IM_COL32(100, 200, 255, 255); // Light blue
            }
            
            // Draw node circle
            draw_list->AddCircleFilled(screen_pos, screen_radius, node_color);
            draw_list->AddCircle(screen_pos, screen_radius, IM_COL32(255, 255, 255, 255), 0, 2.0f);
            
            // Draw deity name
            if (id < deityData.names.size()) {
                const char* name = deityData.names[id].c_str();
                ImVec2 text_size = ImGui::CalcTextSize(name);
                draw_list->AddText(
                    ImVec2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f),
                    IM_COL32(255, 255, 255, 255),
                    name
                );
            }
        }
    }
    
    void HandleInput(ImVec2 canvas_pos, ImVec2 canvas_size) {
        ImGuiIO& io = ImGui::GetIO();
        
        // Check hover
        hoveredDeity.index = (uint16_t)-1;
        ImVec2 mouse_pos = io.MousePos;
        
        if (mouse_pos.x >= canvas_pos.x && mouse_pos.x <= canvas_pos.x + canvas_size.x &&
            mouse_pos.y >= canvas_pos.y && mouse_pos.y <= canvas_pos.y + canvas_size.y) {
            
            for (auto& [id, node] : nodes) {
                ImVec2 screen_pos = WorldToScreen(node.position);
                float screen_radius = node.radius * graphScale;
                
                float dx = mouse_pos.x - screen_pos.x;
                float dy = mouse_pos.y - screen_pos.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                
                if (dist <= screen_radius) {
                    hoveredDeity.index = id;
                    hoveredDeity.generation = node.handle.generation;
                    
                    // Handle click
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        selectedDeity = hoveredDeity;
                        node.isDragging = true;
                    }
                    break;
                }
            }
        }
        
        // Handle dragging
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            for (auto& [id, node] : nodes) {
                if (node.isDragging) {
                    ImVec2 delta = io.MouseDelta;
                    node.position.x += delta.x / graphScale;
                    node.position.y += delta.y / graphScale;
                    node.velocity = ImVec2(0, 0);
                }
            }
        }
        
        // Stop dragging
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            for (auto& [id, node] : nodes) {
                node.isDragging = false;
            }
        }
    }
    
    void DrawLegend() {
        ImGui::Separator();
        ImGui::Text("Relationship Types:");
        ImGui::BeginGroup();
        
        const DeityRelationship relationships[] = {
            DeityRelationship::ParentOf,
            DeityRelationship::SpouseOf,
            DeityRelationship::SiblingOf,
            DeityRelationship::EnemyOf,
            DeityRelationship::ServantOf,
            DeityRelationship::ProtectorOf,
            DeityRelationship::CreatorOf,
            DeityRelationship::SlayerOf,
            DeityRelationship::AuthorOf,
            DeityRelationship::MutilatorOf
        };
        
        for (auto rel : relationships) {
            ImU32 color = GetRelationshipColor(rel);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            
            draw_list->AddLine(
                ImVec2(pos.x, pos.y + 7),
                ImVec2(pos.x + 30, pos.y + 7),
                color, 3.0f
            );
            
            ImGui::Dummy(ImVec2(35, 15));
            ImGui::SameLine();
            ImGui::Text("%s", GetRelationshipName(rel));
        }
        
        ImGui::EndGroup();
    }
    
    void DrawDeityDetails() {
        ImGui::Begin("Deity Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        if (!religionManager->IsValid(currentReligion) || !selectedDeity.isValid()) {
            ImGui::Text("No deity selected");
            ImGui::End();
            return;
        }
        
        auto& deityData = religionManager->GetDeityData();
        auto& relData = religionManager->GetDeityRelationshipData();
        auto& definitions = religionManager->GetReligionDefinitions();
        uint16_t idx = selectedDeity.index;
        
        if (idx >= deityData.names.size()) {
            ImGui::Text("Invalid deity");
            ImGui::End();
            return;
        }
        
        ImGui::Text("Name: %s", deityData.names[idx].c_str());
        
        // Archetype
        if (idx < deityData.archetype_ids.size()) {
            uint16_t arch_id = deityData.archetype_ids[idx];
            if (arch_id < definitions.deity_archetypes.size()) {
                ImGui::Text("Archetype: %s", definitions.deity_archetypes[arch_id].name.c_str());
            }
        }
        
        // Domains
        ImGui::Separator();
        ImGui::Text("Domains:");
        if (idx < deityData.domains.size()) {
            for (auto domain : deityData.domains[idx]) {
                const char* domain_names[] = {
                    "War", "Fertility", "Wisdom", "Love", "Nature", "Sea", "Forests",
                    "Underworld", "Horses", "Metals", "Storms", "Music", "Trade", "Wind",
                    "Fire", "Death", "Darkness", "Sun", "Travel", "Moon", "Order", "Chaos",
                    "Mountains", "Winter", "Water", "Law"
                };
                int domain_idx = static_cast<int>(domain);
                if (domain_idx >= 0 && domain_idx < IM_ARRAYSIZE(domain_names)) {
                    ImGui::BulletText("%s", domain_names[domain_idx]);
                }
            }
        }
        
        // Relationships
        ImGui::Separator();
        ImGui::Text("Relationships:");
        
        uint32_t offset = deityData.relationship_offsets[idx];
        uint32_t count = deityData.relationship_counts[idx];
        
        if (count == 0) {
            ImGui::Text("  None");
        } else {
            for (uint32_t i = 0; i < count; i++) {
                uint32_t rel_idx = offset + i;
                uint16_t target_id = relData.target_deity_ids[rel_idx];
                DeityRelationship rel_type = relData.relationship_types[rel_idx];
                float strength = relData.relationship_strengths[rel_idx];
                
                if (target_id < deityData.names.size()) {
                    ImU32 color = GetRelationshipColor(rel_type);
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::BulletText("%s of %s (%.1f)", 
                        GetRelationshipName(rel_type),
                        deityData.names[target_id].c_str(),
                        strength);
                    ImGui::PopStyleColor();
                }
            }
        }
        
        ImGui::End();
    }
    
    ImVec2 WorldToScreen(ImVec2 world_pos) {
        return ImVec2(
            graphCenter.x + world_pos.x * graphScale,
            graphCenter.y + world_pos.y * graphScale
        );
    }
};
