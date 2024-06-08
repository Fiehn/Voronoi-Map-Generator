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
    unsigned int id; // Unique Id coming from the points vector
    Cell(int i) : id(i) { vertex.reserve(10); neighbors.reserve(10); }; // Constructor, am I doing this right?
    std::vector<int> vertex; // Id's of vertecies that corespond to the cell and are stored in voroi_points this should be pointers?
    std::vector<int> neighbors; // Id's of the neighbors
    unsigned int vertex_offset = 0U; // Offset for the vertex buffer

    float height = 0.f; // Height of the cell, 1 = 8km above sealevel 
    float rise = 0.f; // Difference in height between the highest and the lowest neighbor cell (0 to 1)
    float temp = 0.f; // Temperature of the cell (Celsius)
    float windDir = 0.f; // Wind direction (0 to 360 degrees)
    float windStr = 0.f; // Wind strength (0 to 1)
    float humidity = 1.f; // Humidity of the cell (0 to 1)
    float percepitation = 0.f; // Percepitation of the cell ( > 0 )

    float distToOcean = std::numeric_limits<float>::max();

    bool oceanBool = false; // Is an ocean
    bool coastBool = false; // Is next to ocean

    bool riverBool = false; // Has a river
    float riverStr = 0.f; // River strength

    bool lakeBool = false; // Has a lake
    bool snowBool = false; // Has snow
    bool treeBool = true; // Has trees
    
    bool iceBool = false; // Is Ice cap

    void sort_angles(const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voroi_points);
    
    bool contains(sf::Vector2f point, const std::vector<sf::Vector2f>& voroi_points);

    ~Cell() {
        vertex.clear();
        neighbors.clear();
    }
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

void rise(std::vector<Cell>& map)
{ /* Calculate the rise by finding the tallest and shortest neighbor*/
    // Needs to be optimized or rethought
    for (size_t i = 0; i < map.size(); i++)
    {
        float max_height = std::numeric_limits<float>::min();
        float min_height = std::numeric_limits<float>::max();

        // Cache neighbor heights
        const std::vector<int>& neighbors = map[i].neighbors;
        std::vector<float> neighbor_heights(neighbors.size());
        for (size_t j = 0; j < neighbors.size(); j++) {
			neighbor_heights[j] = map[neighbors[j]].height;
		}

        // find min and max height
        for (int j = 0; j < neighbors.size(); j++)
        {
            float neighbor_height = neighbor_heights[j];
            if (neighbor_height < min_height) min_height = neighbor_height;
            if (neighbor_height > max_height) max_height = neighbor_height;
        }
        map[i].rise = max_height - min_height;
    }
}

// k-point smooth height generator
// there is a max of RAND_MAX_LONG (about a million cells)
void random_height_gen(std::vector<Cell>& map, int k=5, float delta_max_neg=0.04,float delta_max_pos=0.03,float prob_of_island= 0.008,float dist_from_mainland = 1.0, int method = 1)
{   // Method 1 is random, method 2 is first in first out
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
        if(method== 1) { index = pop_random_i(active); }
        else if(method == 2) { index = pop_front_i(active); }

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


void smooth_height(std::vector<Cell>& map, float rise_threshold = 0.1, int repeats = 1, int method = 1)
{ // method 1 = Random, method 2 = Front
    {
        std::vector<unsigned int> active;
        active.reserve(map.size() * 5);

        for (int _ = 0; _ < repeats; _++)
        {
            for (size_t i = 0; i < map.size(); i++)
            {
                if (map[i].rise > rise_threshold)
                {
                    active.push_back(i);
                    active.insert(std::end(active), std::begin(map[i].neighbors), std::end(map[i].neighbors));
                }
            }
            while (true)
            {
                if (active.empty()) { break; }

                size_t index = 0;
                if (method == 1) { index = pop_random_i(active); }
                else if (method == 2) { index = pop_front_i(active); }

                float height_sum = 0.f;
                int count_values = 0;
                for (int j = 0; j < map[index].neighbors.size(); j++)
                {
                    height_sum = height_sum + map[map[index].neighbors[j]].height;
                    count_values++;
                }

                if (height_sum > 0.005) {
                    map[index].height = height_sum / static_cast<float>(count_values);
                }
                else if (map[index].height > 0.5) {
                    map[index].height = 0.1;
                }
            }
            // calculate the new rise
            rise(map);
        }
    }
}

void noise_height(std::vector<Cell>& map, int n)
{
    size_t size = map.size();
    for (int i = 0; i < n; i++)
    {
        for (size_t j = 0; j < size; j++)
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
            map[i].distToOcean = 0;
        }
        else { map[i].oceanBool = false; }
        if (map[i].height <= globals.seaLevel + delta && map[i].height >= globals.seaLevel - delta) 
        { 
            map[i].coastBool = true;
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

void calcTemp(std::vector<Cell>& map, GlobalWorldObjects& globals, const std::vector<sf::Vector2f>& points, const int MAXHEIGHT)
{
    for (int i = 0; i < map.size(); i++)
    {   // Take distance to equator and get the distance 
        // Add an altitute modifier
        // Ocean Currents (needs implementation)
        // Humidity, will be loop (needs implementation)
        // Distance from sea (needs a function that is quick)
        float temp = 0.f;

        // Latitute
        float dist = std::abs(points[i].y - MAXHEIGHT/2);
        float c = 0.0015;
        float b = 5;
        float a = 5;
        temp += 1.5 * globals.globalTempAvg - globals.globalTempAvg * (a * expf(-b * expf(-c * dist))); // Gompertz function
        // Altitute
        if (map[i].height >= globals.seaLevel)
        {
            temp += map[i].height * (-50);
        }
        else
        {
            temp += (1 - map[i].height) * (-50);
        }
        map[i].temp = temp;
    }
}

void smoothTemps(std::vector<Cell>& map,int smoothTimes)
{
    for (int j = 0; j < smoothTimes; j++)
    {
        for (int i = 0; i < map.size(); i++)
        {
            float temp = 0;
            for (int j = 0; j < map[i].neighbors.size(); j++)
            {
                temp += map[map[i].neighbors[j]].temp;
            }
            map[i].temp = temp / map[i].neighbors.size();
        }
    }
}


void calcPercepitation(std::vector<Cell>& map, const std::vector<sf::Vector2f>& points, GlobalWorldObjects& globals)
{ // Humidity, temperature, distance from sea, altitude, ocean currents (warmer=more), wind
    std::vector<int> queue = globals.oceanCells;
    std::vector<bool> visited(map.size(), false);
    while (!queue.empty())
    {
		int idx = pop_front_i(queue);

        if (map[idx].oceanBool) {
            // Really just based on Azgaar.. Should be changed to something I get.
            map[idx].percepitation = ((700 * (map[idx].temp + 0.06 * map[idx].height)) / 50 + 75) / (80 - map[idx].temp);
            for (int i = 0; i < map[idx].neighbors.size(); i++)
            { // Get the wind direction and add fragments of the percepitation to the neighbors based on the wind direction and strength
                float dirOfNeighbor = atan2(points[map[idx].neighbors[i]].y - points[idx].y, points[map[idx].neighbors[i]].x - points[idx].x);
                // Assign similarity of direction to the wind direction
                float simDir = 1 - clamp(cos(dirOfNeighbor - radians(map[idx].windDir)), 1.f, 0.f);

                map[map[idx].neighbors[i]].percepitation += map[idx].percepitation * simDir * map[idx].windStr;

                if (visited[map[idx].neighbors[i]] == false)
                {
					visited[map[idx].neighbors[i]] = true;
                    queue.push_back(map[idx].neighbors[i]);
                }
            }
        }
        else
        {
            for (int i = 0; i < map[idx].neighbors.size(); i++)
            { // Get the wind direction and add fragments of the percepitation to the neighbors based on the wind direction and strength
                float dirOfNeighbor = atan2(points[map[idx].neighbors[i]].y - points[idx].y, points[map[idx].neighbors[i]].x - points[idx].x);
                // Assign similarity of direction to the wind direction
                float simDir = 1 - clamp(cos(dirOfNeighbor - radians(map[idx].windDir)),1.f,0.f);

                map[map[idx].neighbors[i]].percepitation += map[idx].percepitation * simDir * map[idx].windStr;

                if (visited[map[idx].neighbors[i]] == false)
                {
					visited[map[idx].neighbors[i]] = true;
                    queue.push_back(map[idx].neighbors[i]);
                }
            }
        }
        
	}
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

void calcWind(std::vector<Cell>& map, const std::vector<sf::Vector2f>& points, const int MAXHEIGHT, GlobalWorldObjects& globals)
{
    std::vector<float> wind = scalarMultiplication(globals.convergenceLines, (float)MAXHEIGHT);

    for (int i = 0; i < map.size(); i++)
    {
        int closestLine = 0;
        float min_dist = 1000000.f;
        for (int j = 0; j < wind.size(); j++)
        {
            float dist = points[map[i].id].y - wind[j];
            if (abs(dist) < min_dist)
            {
                if (dist < 0)
                {
                    min_dist = abs(dist);
                    closestLine = j - 1;
                }
                else
                {
                    min_dist = abs(dist);
                    closestLine = j;
                }
			}
        }

        map[i].windDir = normalizeAngle(globals.windDirection[closestLine] + 360.f * RandomBetween(-0.3,0.3));
        map[i].windStr = clamp(globals.windStrength[closestLine] + RandomBetween(-0.5,0.5), 1.f, 0.f);
	}
    // get averages of neighbors direction and strength
    for (int i = 0; i < map.size(); i++)
    {
        float sumX = 0.0;
        float sumY = 0.0;
        float sumStr = 0.0;
        for (int j = 0; j < map[i].neighbors.size(); j++)
        {
            float radian = radians(map[map[i].neighbors[j]].windDir);
            sumX += std::cos(radian);
            sumY += std::sin(radian);

            sumStr += map[map[i].neighbors[j]].windStr;
        }
        float averageRadians = std::atan2f(sumY, sumX);
        map[i].windDir = normalizeAngle(averageRadians * 180.0 / PI);
        map[i].windStr = sumStr / map[i].neighbors.size();
    }
}


void calcSnow(std::vector<Cell>& map, GlobalWorldObjects& globals)
{

}


// Will Queue up all ocean cells then evaluate neighbors and que up any that are smaller than the current distance
void closeOceanCell(std::vector<Cell>& map, const std::vector<sf::Vector2f>& points, const GlobalWorldObjects& globals)
{
    std::vector<int> queue = globals.oceanCells;
    while (!queue.empty())
    {
		int idx = pop_front_i(queue);
        for (int i = 0; i < map[idx].neighbors.size(); i++)
        {
            if (!map[map[idx].neighbors[i]].oceanBool)
            {
                // Assign coastBool if not already assigned
                map[map[idx].neighbors[i]].coastBool = map[idx].oceanBool;
                // Calculate distance to Ocean
                float distance = dist(points[map[idx].neighbors[i]], points[idx]) + map[idx].distToOcean;
                if (map[map[idx].neighbors[i]].distToOcean > distance)
                {
                    map[map[idx].neighbors[i]].distToOcean = distance;
                    queue.push_back(map[idx].neighbors[i]);
                }
			}
		}
	}
}

