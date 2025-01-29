#pragma once
#include "cell.hpp"
#include <cmath>
#include <algorithm>

// contains
bool Cell::contains(sf::Vector2f point, const std::vector<sf::Vector2f>& voroi_points)
{
    bool result = false;
    for (size_t i = 0, j = vertex.size() - 1; i < vertex.size(); j = i++) {
        if ((voroi_points[vertex[i]].y > point.y) != (voroi_points[vertex[j]].y > point.y) &&
            (point.x < (voroi_points[vertex[j]].x - voroi_points[vertex[i]].x) * (point.y - voroi_points[vertex[i]].y) / (voroi_points[vertex[j]].y - voroi_points[vertex[i]].y) + voroi_points[vertex[i]].x)) {
            result = !result;
        }
    }
    return result;
}

// Sort angles between center, point and horizontal for drawing triangles (insertion sort)
void Cell::sort_angles(const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voroi_points)
{
    // Create a temporary vector to store vertex indices along with their corresponding angles
    std::vector<std::pair<int, float>> vertex_with_angles;
    vertex_with_angles.reserve(vertex.size());

    // Calculate angles and store them along with vertex indices
    for (size_t i = 0; i < vertex.size(); i++) {
        float angle = static_cast<float>(atan2(points[id].y - voroi_points[vertex[i]].y, points[id].x - voroi_points[vertex[i]].x));
        vertex_with_angles.emplace_back(vertex[i], angle);
    }

    // Sort the vertex_with_angles vector based on angles
    std::sort(vertex_with_angles.begin(), vertex_with_angles.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second < rhs.second;
        });

    // Update vertex with sorted vertex indices
    for (size_t i = 0; i < vertex.size(); i++) {
        vertex[i] = vertex_with_angles[i].first;
    }
}






