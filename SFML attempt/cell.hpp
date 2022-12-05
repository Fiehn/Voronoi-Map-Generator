#pragma once

#include <SFML/System/Vector2.hpp>
#include <vector>
#include <algorithm> 
#include <cmath>
#include "points.hpp"


class Cell 
{  
public:
    const int id; // Unique Id coming from the points vector
    Cell(int i) : id(i) {}; // Constructor, am I doing this right?
    std::vector<int> vertex; // Id's of vertecies that corespond to the cell and are stored in voroi_points this should be pointers?
    float height = 0.f;
    std::vector<int> neighbors;
    
    void add_neighbors(int &key) {
        // check list for duplicates
        if (!std::count(neighbors.begin(), neighbors.end(), key)) {
            neighbors.push_back(key);
        }
    }
    void add_vertex(int &key) {
        vertex.push_back(key);
    }
    
    void bubble_sort_angles(const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voroi_points);

};

// Sort angles between center, point and horizontal for drawing trianglefans 
void Cell::bubble_sort_angles(const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voroi_points) 
{
    std::vector<float> angle;
    for (size_t i = 0; i < vertex.size(); i++) {
        angle.push_back((float)(atan2(points[id].y - voroi_points[vertex[i]].y, points[id].x - voroi_points[vertex[i]].x)));
    }

    bool sorted = false;
    while (!sorted)
    {
        sorted = true;
        for (int k = 0; k < vertex.size() - 1; ++k)
        {
            if (angle[k] > angle[k + 1])
            {
                float v = angle[k];
                angle[k] = angle[k + 1];
                angle[k + 1] = v;
                int value = vertex[k];
                vertex[k] = vertex[k + 1];
                vertex[k + 1] = value;
                sorted = false;
            }
        }
    }
}




