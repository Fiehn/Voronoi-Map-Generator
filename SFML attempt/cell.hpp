#pragma once
#include <vector>
#include <algorithm> 
#include <iostream>
#include <cmath>
#include <numeric>
#include "util.h"
#include "Voronoi.hpp"
#include "GlobalWorldObjects.hpp"

class Cell 
{  
public:
    const int id; // Unique Id coming from the points vector
    Cell(int i) : id(i) { vertex.reserve(15); neighbors.reserve(15); }; // Constructor, am I doing this right?
    std::vector<int> vertex; // Id's of vertecies that corespond to the cell and are stored in voroi_points this should be pointers?
    float height = 0.f;
    std::vector<int> neighbors;
    float rise = 0.f;
    float avgTemp = 0.f;
    float tempSTD = 1.f;

    bool oceanBool = false;
    bool coast = false;

    bool riverBool = false;
    float riverStr = 0.f;

    bool lakeBool = false;
    bool snowBool = false;
    bool treeBool = true;
    
    bool iceBool = false;

    void sort_angles(const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voroi_points);
    
    bool contains(sf::Vector2f point, const std::vector<sf::Vector2f>& voroi_points);
};

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

// Sort angles between center, point and horizontal for drawing trianglefans (insertion sort)
void Cell::sort_angles(const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voroi_points)
{
    std::vector<float> angle;
    angle.reserve(vertex.size());
    for (size_t i = 0; i < vertex.size(); i++) {
        angle.push_back((float)(atan2(points[id].y - voroi_points[vertex[i]].y, points[id].x - voroi_points[vertex[i]].x)));
    }
    
    for (size_t i = 1; i < angle.size(); i++) {
		float key = angle[i];
		int key2 = vertex[i];
		int j = i - 1;
        while (j >= 0 && angle[j] > key) {
			angle[j + 1] = angle[j];
			vertex[j + 1] = vertex[j];
			j = j - 1;
		}
		angle[j + 1] = key;
		vertex[j + 1] = key2;
	}
}

void rise(std::vector<Cell>& map)
{
    /* Calculate the rise by finding the tallest and shortest neighbor*/
    for (size_t i = 0; i < map.size(); i++)
    {
        float max_height = std::numeric_limits<float>::min();
        float min_height = std::numeric_limits<float>::max();
        for (int j = 0; j < map[i].neighbors.size(); j++)
        {
            float neighbor_height = map[map[i].neighbors[j]].height;
            if (neighbor_height < min_height) min_height = neighbor_height;
            if (neighbor_height > max_height) max_height = neighbor_height;
        }
        map[i].rise = max_height - min_height;
    }
}

// k-point smooth height generator
// there is a max of RAND_MAX_LONG (about a million cells)
void random_height_gen(std::vector<Cell>& map, int k=5, float delta_max_neg=0.04,float delta_max_pos=0.03,float prob_of_island= 0.008,float dist_from_mainland = 1.0,std::string method = "Front")
{   
    /* 
    1. Initiate queue active
    2. Pick k random starting cells
    3. Assign height above 0.9 to these cells and add their neighbors to active
    4.  Check neighbor's height
        if not 0 save it else add to active if not already there
        add small probability of height increase for each neighbor 
    5. Calculate neighbor averages and add random factor
    6. Go back to step 4 until active is empty
    */
    // Active cells that have not been assigned a height yet, should be a queue of some sort
    std::vector<int> active;

    for (int i = 0; i < k; i++)
    {
        int index = rand_long() % map.size();
        map[index].height = RandomBetween(0.8, 1.0);
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
                map[map[index].neighbors[j]].height = RandomBetween(0.6, 0.9);
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
                insert_unique(active, map[index].neighbors[j]);
                //if (std::find(active.begin(), active.end(), map[index].neighbors[j]) == active.end()) {
                //    active.push_back(map[index].neighbors[j]);
                //}
            }
        }
        if (count_values == 0) { active.push_back(map[index].id); }
        else {
            map[index].height = clamp((height_sum / count_values) + RandomBetween(-delta_max_neg, delta_max_pos), 1.0, 0.0);
        }
    }
    rise(map); // calculate the rise of the map with the new height values
}


void smooth_height(std::vector<Cell>& map,float rise_threshold=0.1,int repeats = 1, std::string method="Random")
{
    for (int _ = 0; _ < repeats; _++)
    {
        std::vector<int> active;
        for (size_t i = 0; i < map.size(); i++)
        {
            if (map[i].rise > rise_threshold)
            {
                active.push_back(i);
                active.insert(std::end(active), std::begin(map[i].neighbors), std::end(map[i].neighbors));
            }
        }
        while (active.empty() == false)
        {
            int index = 0;
            if (method == "Random") { index = pop_random_i(active); }
            else if (method == "Front") { index = pop_front_i(active); }

            float height_sum = 0.f;
            int count_values = 0;
            for (int j = 0; j < map[index].neighbors.size(); j++)
            {
                height_sum = height_sum + map[map[index].neighbors[j]].height;
                count_values++;
            }

            if (height_sum > 0.005) {
                map[index].height = height_sum / (float)count_values;
            }
            else if (map[index].height > 0.5) {
                map[index].height = 0.1;
            }
        }
        // calculate the new rise
        rise(map);
    }
}

void noise_height(std::vector<Cell>& map, int n)
{
    for (int i = 0; i < n; i++)
    {
        for (size_t j = 0; j < map.size(); j++)
        {
			map[j].height = map[j].height + RandomBetween(-0.005, 0.005);
		}
	}
}

void calcHeightValues(std::vector<Cell>& map, GlobalWorldObjects& globals, float delta) // sea level, coast, treeline and snow line
{
    for (size_t i = 0; i < map.size(); i++)
    {
        map[i].height = clamp(map[i].height, 1.0, 0.0);

        if (map[i].height <= globals.seaLevel) 
        {
            map[i].oceanBool = true;
            globals.oceanCells.push_back(i);
        }
        else { map[i].oceanBool = false; }
        if (map[i].height <= globals.seaLevel + delta && map[i].height >= globals.seaLevel - delta) 
        { 
            map[i].coast = true;
            globals.coastCells.push_back(i);
        }
        if (map[i].height >= globals.globalSnowline) 
        {
            map[i].snowBool = true;
            globals.snowCells.push_back(i);
        }
        if (map[i].height >= globals.globalTreeline) 
        {
            map[i].treeBool = false;
            globals.treeCells.push_back(i);
        }
    }
}

// Rivers

// First check if cells over snow line have neighbors below snow line and no rivers in their neighbors
// Then start a river from that cell and add river to the neighbor and that one
// Then add to the steepest neighbor and so on until you reach the sea or a lake
// If you reach a lake, add the river to the lake and then continue from the lake

void riverIteration(std::vector<Cell>& map, GlobalWorldObjects& globals, std::vector<int>& stack, int start)
{
    std::vector<std::pair<int,float>> HeightDiff;
    HeightDiff.reserve(map[start].neighbors.size());
    for (int i = 0; i < map[start].neighbors.size(); i++)
    {
        HeightDiff.push_back({ map[start].neighbors[i], map[start].height - map[map[start].neighbors[i]].height });
    }
    std::sort(HeightDiff.begin(), HeightDiff.end(), [](const std::pair<int, float>& a, const std::pair<int, float>& b) { return a.second < b.second; });

    int order = 1;

    if (HeightDiff[HeightDiff.size() - 1].second < 0) // ITs too happy to make lakes, it needs more rivers.
    { // if no neighbor is found to have a lower height, either the river ends or it should generate a lake
      // Determined by the sum of nearby differences
        // TODO: This needs to be tuned and fixed
        float sum = 0;
        for (const auto& pair : HeightDiff) {
            sum += std::abs(pair.second);
        }
        if (sum <= 0.1)
        {
            for (int i = 0; i < map[start].neighbors.size(); i++)
            {
                map[map[start].neighbors[i]].lakeBool = true;
            }
        }
    }
    else
    {
        if (map[HeightDiff[HeightDiff.size() - order].first].snowBool == true)
        {
            return;
        }
        else if (map[HeightDiff[HeightDiff.size() - order].first].oceanBool == true)
        {
            return;
        }
        
        else if (map[HeightDiff[HeightDiff.size() - order].first].riverBool == true)
        {// pick a neighbor whose neighbor's neighbors are not rivers
            std::vector<int> possibleNeighbors;
            for (int i = 0; i < map[start].neighbors.size(); i++)
            {
                if (map[map[start].neighbors[i]].riverBool == false) 
                {
                    int countRiverNeighbors = 0;
                    for (int j = 0; j < map[map[start].neighbors[i]].neighbors.size(); j++)
                    {
                        if (map[map[map[start].neighbors[i]].neighbors[j]].riverBool) { countRiverNeighbors += 1; }
                    }
                    if (countRiverNeighbors <= 1)
                    {
                        possibleNeighbors.push_back(map[start].neighbors[i]);
                    }
                }
            }
            if (possibleNeighbors.size() > 0)
            {
                std::sort(possibleNeighbors.begin(), possibleNeighbors.end());
				map[possibleNeighbors[possibleNeighbors.size() - 1]].riverBool = true;
				globals.riverCells.push_back(possibleNeighbors[possibleNeighbors.size() - 1]);
				map[possibleNeighbors[possibleNeighbors.size() - 1]].riverStr = map[start].riverStr; //- RandomBetween(0.001, 0.003);
				riverIteration(map, globals, stack, possibleNeighbors[possibleNeighbors.size() - 1]);
            }
            else {
                
                return;
            }
        }
    }

    map[HeightDiff[HeightDiff.size() - order].first].riverBool = true;
    globals.riverCells.push_back(HeightDiff[HeightDiff.size() - order].first);
    map[HeightDiff[HeightDiff.size() - order].first].riverStr = map[start].riverStr; //- RandomBetween(0.001, 0.003);
    riverIteration(map, globals, stack, HeightDiff[HeightDiff.size() - order].first);
    // FLow strenght calculate here

}

void calcRiverStart(std::vector<Cell>& map, GlobalWorldObjects& globals)
{
    // copy vector 1 to vector 2
   
    std::vector<int> stack;
    stack.reserve(globals.snowCells.size());
    for (int i = 0; i < globals.snowCells.size(); i++)
    {
        int countNonSnow = 0;

        for (int j = 0; j < map[globals.snowCells[i]].neighbors.size(); j++)
        {
            if (!map[map[globals.snowCells[i]].neighbors[j]].snowBool)
            {
                countNonSnow = countNonSnow + 1;
            }
        }
        if (countNonSnow >= 2) 
        {
            stack.push_back(globals.snowCells[i]);
        }
    }

    while (!stack.empty())
    {
        int count = 0; 
        int idx = pop_random_i(stack);
        if (map[idx].riverBool == true)
        {
            continue;
        }
        if (map[idx].lakeBool == true)
        {
            continue;
        }
        for (int i = 0; i < map[idx].neighbors.size(); i++)
        {
            if (map[map[idx].neighbors[i]].riverBool == true)
            {
                count++;
            }
        }
        if (count >= 1)
        {
			continue;
		}
        if (RandomBetween(0.0, 1.0) > 0.4)
        {
            globals.riverCells.push_back(idx);
            map[idx].riverBool = true;
            map[idx].riverStr = RandomBetween(0.99, 1.0); // has to be based on temperature and percepitation as well
            riverIteration(map, globals, stack, idx);
        }
    }
}

void calcPreassure(std::vector<Cell>& map, GlobalWorldObjects& globals)
{

}


void calcPercepitation(std::vector<Cell>& map)
{

}

void calcHumid(std::vector<Cell>& map)
{

}

void calcBiome(std::vector<Cell>& map)
{

}

void calcLakes(std::vector<Cell>& map)
{

}

void calcWind(std::vector<Cell>& map)
{

}