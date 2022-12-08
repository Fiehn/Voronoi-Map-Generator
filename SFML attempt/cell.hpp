#pragma once

#include <SFML/System/Vector2.hpp>
#include <vector>
#include <algorithm> 
#include <cmath>
#include <numeric>
#include "points.hpp"

float RandomBetween(float smallNumber, float bigNumber)
{
    float diff = bigNumber - smallNumber;
    return (((float)rand() / RAND_MAX) * diff) + smallNumber;
}

int pop_front_i(std::vector<int>& v)
{
    if (v.empty()) { return 0; }

    int value = v[0];
    v.erase(v.begin());
    return value;

}

int pop_random_i(std::vector<int>& v)
{
    if (v.empty()) { return 0; }

    int rand_index = rand() % v.size();
    int value = v[rand_index];
    v.erase(std::next(v.begin(),rand_index));
    return value;
}

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


// Height tests
// 
// 
// Damn.. RAND_MAX is usually about 32k, waaay too little for +100k cells... figure something out.
// k-point smooth height generator
void height_gen(std::vector<Cell>& map, int k=5, float delta_max_neg=0.04,float delta_max_pos=0.03,float prob_of_island= 0.008,float dist_from_mainland = 1.0,std::string method = "Front")
{   
    /* 
    1. Initiate queue active
    2. Pick k random starting cells
    3. Assign height above 0.9 to these cells and add their neighbors to active
    4.
        Check neighbor's height
        if not 0 save it else add to active if not already there
        add small probability of height increase for each neighbor 
    5. Calculate neighbor averages and add random factor
    6. Go back to step 4 until active is empty
    */
    // Active cells that have not been assigned a height yet, should be a queue of some sort
    std::vector<int> active;

    for (int i = 0; i < k; i++)
    {
        int index = rand() % map.size();
        map[index].height = RandomBetween(0.90, 1.0);
        active.insert(std::end(active), std::begin(map[index].neighbors), std::end(map[index].neighbors));
    }

    while (active.empty() == false)
    {
        int index = 0;
        if(method=="Random") { index = pop_random_i(active); }
        else if(method =="Front") { index = pop_front_i(active); }

        float height_sum = 0.0;
        int count_values = 0;
        for (int j = 0; j < map[index].neighbors.size(); j++)
        {
            // small probability of random height increase, THIS is heavily up to tuning for interesting maps
            // Also should be reconsidered
            if (RandomBetween(0.0, 1.0) < prob_of_island && height_sum < dist_from_mainland && count_values > 1)
            {
                map[map[index].neighbors[j]].height = RandomBetween(0.4, 0.6);
                active.insert(std::begin(active), std::begin(map[map[index].neighbors[j]].neighbors), std::end(map[map[index].neighbors[j]].neighbors));
            }
            if (map[map[index].neighbors[j]].height != 0.f)
            {
                height_sum = height_sum + map[map[index].neighbors[j]].height;
                count_values++;
            }
            else
            {
                // Slow and not very readable, moving the code a bit could make it faster as well
                // Checks for duplicates then adds to active
                if (std::find(active.begin(), active.end(), map[index].neighbors[j]) == active.end()) {
                    active.push_back(map[index].neighbors[j]);
                }
            }
        }
        if (count_values == 0) { active.push_back(map[index].id); }
        map[index].height = clamp((height_sum / count_values) + RandomBetween(-delta_max_neg,delta_max_pos),1.0,0.0);

    }

}


