#pragma once
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <map>
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include "cell.hpp"
#include "GlobalWorldObjects.hpp"
#include "clustering.hpp"
#include <SFML/System/Vector2.hpp>
#include "util.hpp"

void rise(std::vector<Cell>& map)
{ /* Calculate the rise by finding the tallest and shortest neighbor*/
    // Needs to be optimized or rethought
    float max_height = std::numeric_limits<float>::min();
    float min_height = std::numeric_limits<float>::max();

#pragma omp parallel for num_threads(16) schedule(static)
    for (size_t i = 0; i < map.size(); i++)
    {
        max_height = std::numeric_limits<float>::min();
        min_height = std::numeric_limits<float>::max();

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

#pragma omp critical
        map[i].rise = max_height - min_height;
    }
}

// k-point smooth height generator
// there is a max of RAND_MAX_LONG (about a million cells)
void random_height_gen(std::vector<Cell>& map, int k = 5, float delta_max_neg = 0.04, float delta_max_pos = 0.03, float prob_of_island = 0.008, float dist_from_mainland = 1.0, int method = 1)
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
    active.reserve(map.size() - 1);

    for (int i = 0; i < k; i++)
    {
        int index = rand_long() % map.size();
        map[index].height = RandomBetween(0.8, 1.0);
        active.insert(std::end(active), std::begin(map[index].neighbors), std::end(map[index].neighbors));
    }

    while (active.empty() == false)
    {
        int index = 0;
        if (method == 1) { index = pop_random_i(active); }
        else if (method == 2) { index = pop_front_i(active); }

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
            // calculate the new rise for next smoothing
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

// Rivers - needs to be rewritten to be more efficient and to add river objects to the map

// First check if cells over snow line have neighbors below snow line and no rivers in their neighbors
// Then start a river from that cell and add river to the neighbor and that one
// Then add to the steepest neighbor and so on until you reach the sea or a lake
// If you reach a lake, add the river to the lake and then continue from the lake
// If you reach the sea, add the river to the sea and stop
void riverIteration(std::vector<Cell>& map, GlobalWorldObjects& globals, std::vector<std::size_t>& stack, std::size_t start, std::size_t river_id)
{
    std::vector<std::pair<std::size_t, float>> HeightDiff;
    HeightDiff.reserve(map[start].neighbors.size());
    float height = map[start].height;
    for (std::size_t i = 0; i < map[start].neighbors.size(); i++)
    {
        HeightDiff.push_back({ map[start].neighbors[i], height - map[map[start].neighbors[i]].height });
    }
    std::sort(HeightDiff.begin(), HeightDiff.end(), [](const std::pair<int, float>& a, const std::pair<int, float>& b) { return a.second < b.second; });

    int order = 1;

    if (HeightDiff[HeightDiff.size() - 1].second < 0) // ITs too happy to make lakes, it needs more rivers.
    {
        // if no neighbor is found to have a lower height, either the river ends or it should generate a lake

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
        // We check if the neighbor is snow, ocean or river
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
                globals.rivers[river_id].addCell(possibleNeighbors[possibleNeighbors.size() - 1]);
				map[possibleNeighbors[possibleNeighbors.size() - 1]].riverId = river_id;
                riverIteration(map, globals, stack, possibleNeighbors[possibleNeighbors.size() - 1], river_id);
            }
            else {

                return;
            }
        }
    }

    map[HeightDiff[HeightDiff.size() - order].first].riverBool = true;
    globals.riverCells.push_back(HeightDiff[HeightDiff.size() - order].first);
    map[HeightDiff[HeightDiff.size() - order].first].riverStr = map[start].riverStr; //- RandomBetween(0.001, 0.003);
    globals.rivers[river_id].addCell(HeightDiff[HeightDiff.size() - order].first);
	map[HeightDiff[HeightDiff.size() - order].first].riverId = river_id;
    riverIteration(map, globals, stack, HeightDiff[HeightDiff.size() - order].first, river_id);

    // FLow strenght calculate here

}

void calcRiverStart(std::vector<Cell>& map, GlobalWorldObjects& globals, const std::vector<sf::Vector2f>& points)
{
    std::vector<std::size_t> stack;

    // Find all snow cells with more than 2 non-snow neighbors
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

    // Start the river from the snow cells
    std::size_t i = 0;
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
            // If this is the place we can start a new river

            globals.rivers.emplace_back(River(i));

            globals.rivers[i].addCell(idx);

            globals.riverCells.push_back(idx);
            map[idx].riverBool = true;
            map[idx].riverStr = RandomBetween(0.99, 1.0); // has to be based on temperature and percepitation as well
			map[idx].riverId = i;
            riverIteration(map, globals, stack, idx, i);
			globals.rivers[i].finishRiver(map, points);
			i++;
        }
    }
}

void calcTemp(std::vector<Cell>& map, GlobalWorldObjects& globals, const std::vector<sf::Vector2f>& points, const int MAXHEIGHT)
{
    for (int i = 0; i < map.size(); i++)
    {   // Take distance to equator and get the distance 
        // Add an altitute modifier
        // Ocean Currents (needs implementation)

        float temp = 0.f;

        // Latitute
        float dist = std::abs(points[i].y - MAXHEIGHT / 2);
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
        // Distance from sea (needs to be tuned)
        temp += map[i].distToOcean * 0.5;

        map[i].temp = temp;
    }
}

void smoothTemps(std::vector<Cell>& map, int smoothTimes)
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

void calcPercepitation(std::vector<Cell>& map, const std::vector<sf::Vector2f>& points, GlobalWorldObjects& globals, int runs = 1)
{
    // Algorithm:
    // 1. Start by assigning a baseline for all cells with ocean cells having larger percepitation
    // 2. Create a queue of ocean cells and set them as visited
    // 3. While the queue is not empty, pop the front cell and add a percepitation increase to all neighbors
    // 4. The percepitation increase is based on the wind direction, wind strength, height, distance to ocean and the percepitation of the current cell
    // 5. Add the neighbors to the queue if they are not visited
    // 6. Repeat until the queue is empty

    for (int j = 0; j < runs; j++)
    {
        if (j == 0)
        {
            for (std::size_t i = 0; i < map.size(); i++)
            {
                map[i].percepitation = 1 * static_cast<float>(map[i].oceanBool) * ((700 * clamp((map[i].temp), 1000.f, 0.f)) / 50 + 125) / (80 - clamp((map[i].temp), 1000.f, 0.f)) + 1 - std::exp(-0.25 * static_cast<float>(map[i].distToOcean));
            }
        }
        std::vector<bool> visited(map.size(), false);
        Queue<std::size_t> queue;
        for (std::size_t i = 0; i < map.size(); i++)
        {
            if (map[i].oceanBool == true)
            {
                queue.push(i);
                visited[i] = true;
            }
        }

        while (!queue.empty())
        {
            // Base truths:
            // 1. You cannot have more percepitation than your neighbors, unless you are an ocean cell
            // 2. You cannot receive percepitation from a cell where the wind is blowing away from you
            // 3. You will increase percepitation when reaching steap inclines but not decrease when close to level ground
            std::size_t i = queue.pop_front();

            std::vector<int> neighbors = map[i].neighbors;
            float windDir = map[i].windDir;
            float windStr = map[i].windStr;
            float height = map[i].height;
            float percepitation = map[i].percepitation;

            // Asserting percepitation maximum transfer. See (1)
            float maxPercepitation = 0;
            if (map[i].oceanBool == false)
            {
                for (int k = 0; k < neighbors.size(); k++)
                {
                    if (map[neighbors[k]].percepitation > maxPercepitation)
                    {
                        maxPercepitation = map[neighbors[k]].percepitation;
                    }
                }
            }
            else { maxPercepitation = 1000; }
            percepitation = std::min(percepitation, maxPercepitation);

            // Convert wind direction to [0, 2PI]
            windDir = radians(windDir);

            for (int k = 0; k < neighbors.size(); k++)
            {
                // Calculates the direction similarity of wind and the neighbor. See (2).
                float atan2Direction = atan2(points[neighbors[k]].y - points[i].y, points[neighbors[k]].x - points[i].x);
                float direction = fmod(atan2Direction + 2 * PI, 2 * PI);
                float simDir = std::abs(direction - windDir);
                if (simDir > PI) { simDir = 2 * PI - simDir; } // Wrapping the smallest angle to [0, PI]
                simDir = (std::cos(simDir) + 1) / 2; // [0, 1]

                // Taking the height of neighbor. See (3).
                // Therefore when this is 0 (flat) the steepness should not affect the percepitation added (=1)
                // When steepness high lower the percepitation added
                float steepness = std::exp(-100 * std::abs(map[neighbors[k]].height - height)); // [0,1]

                // 0.3 is a magic number? 
                // Adding the distance to the ocean factored by the wind strenght to further increase truth 1.
                float neighbor_percepitation = 0.3 * (steepness + simDir) * (percepitation) * (1 - map[neighbors[k]].oceanBool) * std::exp(-0.01 * static_cast<float>(map[i].distToOcean) * (1 - windStr));
                map[neighbors[k]].percepitation += neighbor_percepitation;

                if (visited[neighbors[k]] == false)
                {
                    queue.push(neighbors[k]);
                }
                visited[neighbors[k]] = true;
            }
        }

    }

}

void smoothPercepitation(std::vector<Cell>& map, int smoothTimes)
{
    for (int j = 0; j < smoothTimes; j++)
    {
        for (int i = 0; i < map.size(); i++)
        {
            float percepitation = 0;
            for (int j = 0; j < map[i].neighbors.size(); j++)
            {
                percepitation += map[map[i].neighbors[j]].percepitation;
            }
            map[i].percepitation = percepitation / map[i].neighbors.size();
        }
    }
}

void calcHumid(std::vector<Cell>& map)
{
    for (int i = 0; i < map.size(); i++)
    {
        // smooth function to get the humidity from the percepitation, temperature // Needs work and wind.
        float humid = 1 + exp(-0.2 * (std::logf(map[i].percepitation) + std::logf(std::abs(map[i].temp))));
        if (humid == 0.f || isinf(humid) || isnan(humid))
        {
            humid = 0.5;
        }

        map[i].humidity = 1 / humid;
    }
}

void removeBiome(int id, GlobalWorldObjects& globals, std::vector<Cell>& map)
{
    // if the biome is used in any cell, the cell is set to the default biome
    // all other biomes have their index updated to match the new list and the id of all cells is updated
    // the biome is removed from the list

    for (int i = 0; i < map.size(); i++)
    {
        if (map[i].biome == id)
        {
            map[i].biome = 0;
        }
        else if (map[i].biome > id)
        {
            map[i].biome = (map[i].biome - 1);
        }
    }
    globals.biomes.erase(globals.biomes.begin() + id);
    for (int i = 0; i < globals.biomes.size(); i++)
    {
        globals.biomes[i].setId(i);
    }
}

void calcBiome(std::vector<Cell>& map, GlobalWorldObjects& globals, int kmeans_max_iter = 5, int method = 1, float prob_smoothing = 0.5f) {
    if (globals.biomes.size() == 0) {
        globals.generateBiomes();
    }

    // Variable names for the clusters and biomes
    std::vector<std::string> names = { "Ocean", "Temperature", "Percepitation", "Humidity", "Height", "Wind Strength", "Dist to Ocean" };

    // get vectors of the variables for the biomes
    std::vector<std::vector<float>> temporary;
    temporary.resize(map.size());

    for (int i = 0; i < map.size(); i++) {
        temporary[i].resize(globals.biomes.size(), 0);
        temporary[i] = { map[i].oceanBool * 100.f, map[i].temp, map[i].percepitation, map[i].humidity, map[i].height, map[i].windStr, static_cast<float>(map[i].distToOcean) };
    }

    // initialize a placeholder 
    std::unique_ptr<ClusteringMethod> clusteringMethod;
    bool smoothing = false;

    if (method == 1 || method == 3)
    {
        if (method == 1)
        {
            smoothing = true;
        }
        clusteringMethod = std::make_unique<GMM>(globals.biomes.size(), temporary[0].size(), kmeans_max_iter);
    }
    else if (method == 2)
    {
        smoothing = false;
        clusteringMethod = std::make_unique<KMeans>(globals.biomes.size(), temporary[0].size(), kmeans_max_iter);
    }

    if (clusteringMethod == nullptr) {
        std::cout << "Error: Clustering method not found" << std::endl;
        return;
    }

    clusteringMethod->setData(temporary);
    clusteringMethod->run();

    for (int i = 0; i < map.size(); i++) {
        int cluster = clusteringMethod->getClusterId(i);
        map[i].biome = cluster;
        map[i].biome_prob = clusteringMethod->getBiomeProb(i);
    }

    // set the biomes values to the averages 
    for (int i = 0; i < globals.biomes.size(); i++) {
        std::map<std::string, float> values;
        std::vector<float> biomeValues = clusteringMethod->getCentroid(i);
        for (int j = 0; j < names.size(); j++) {
            values.emplace(names[j], biomeValues[j]);
        }

        globals.biomes[i].setValues(values);
    }

    // remove biomes with 0 cluster size
    std::vector<int> toRemove;
    std::vector<int> clusterSizes = clusteringMethod->getClusterSizes();

    for (int i = 0; i < globals.biomes.size(); i++) {
        if (clusterSizes[i] == 0) {
            toRemove.push_back(i);
        }
    }
    //for (int i = 0; i < toRemove.size(); i++) {
    //    removeBiome(toRemove[i] - i, globals, map);
    //}

    for (int i = 0; i < globals.biomes.size(); i++) {
        globals.biomes[i].numCells = clusterSizes[i];
    }

    // Here we observe the neighbors of each cell and check their biomes, updating the probabilities of a cells biomes and then afterwards taking the new biome with the highest probability
    if (!smoothing) { return; }

    std::vector<bool> ocean_bool;
    for (std::size_t i = 0; i < globals.biomes.size(); i++)
    {
        ocean_bool.push_back(globals.biomes[i].isOcean);
    }

    for (std::size_t i = 0; i < map.size(); i++)
    {
        std::vector<int> neighbors = map[i].neighbors;
        std::vector<float> probs(globals.biomes.size(), 0.f);
        for (std::size_t j = 0; j < neighbors.size(); j++)
        {
            std::vector<float> neighbor_probs = map[neighbors[j]].biome_prob;
            for (std::size_t k = 0; k < probs.size(); k++)
            {
                probs[k] += neighbor_probs[k];
            }
        }
        // Take the average of the neighbors and weight them by smoothing factor
        probs = scalarMultiplication(probs, prob_smoothing / neighbors.size());

        map[i].biome_prob = elementWiseAdd(map[i].biome_prob, probs);

        // make it a probability again
        map[i].biome_prob = scalarMultiplication(map[i].biome_prob, 1.f / (sum_vec_float(map[i].biome_prob) + 1e-8));

        booleanMapVector_f(map[i].biome_prob, ocean_bool, !map[i].oceanBool);
    }
    // reset biomes sizes
    for (std::size_t i = 0; i < globals.biomes.size(); i++)
    {
        globals.biomes[i].numCells = 0;
    }
    // now find the highest probability and set the biome to that
    for (std::size_t i = 0; i < map.size(); i++)
    {
        std::vector<float> probs = map[i].biome_prob;
        map[i].biome = chooseIndexMax(probs);

        globals.biomes[map[i].biome].numCells += 1;
    }
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

        map[i].windDir = normalizeAngle(globals.windDirection[closestLine] + 360.f * RandomBetween(-0.3, 0.3));
        map[i].windStr = clamp((globals.windStrength[closestLine] + RandomBetween(-0.5, 0.5)) * (1 - clamp(map[i].height, 0.6f, 0.4f)) * 2, 1.f, 0.f);
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

void calcIce(std::vector<Cell>& map, GlobalWorldObjects& globals)
{

}

void closeOceanCell(std::vector<Cell>& map, const GlobalWorldObjects& globals)
{   // Start at every ocean cell, set them in the queue. 
    // Go through neighbors and check if any neighbors are land.
    // If they are, set the first to a coast cell. Then set their distance to ocean to 1.
    // Then add the neighbors to the queue.
    // If the current cell is a land cell, check neighbors distance to ocean 
    // and set the distance to ocean to either the current cells distance + 1 or the neighbors current distance.
    // Then add the neighbors to the queue.
    Queue<int> queue;

    std::vector<bool> visited(map.size(), false);

    // Add all ocean cells to the queue
    for (int i = 0; i < globals.oceanCells.size(); i++)
    {
        queue.push(globals.oceanCells[i]);
        map[globals.oceanCells[i]].distToOcean = 0;
        visited[globals.oceanCells[i]] = true;
    }
    while (!queue.empty())
    {
        int idx = queue.pop_random();
        visited[idx] = true;

        int curr_distance = map[idx].distToOcean;
        if (curr_distance == 1 && map[idx].coastBool==false && map[idx].oceanBool == false)
        {
			map[idx].coastBool = true;
		}

        for (int i = 0; i < map[idx].neighbors.size(); i++)
        {
            int neighbor = map[idx].neighbors[i];
            
            if (map[neighbor].distToOcean > curr_distance + 1)
            {
                map[neighbor].distToOcean = curr_distance + 1;
            }
            if (visited[neighbor] == false)
            {
                queue.push(neighbor);
            }
        }
    }


}

