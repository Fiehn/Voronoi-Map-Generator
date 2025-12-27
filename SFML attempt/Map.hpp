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

void continent_generation(std::vector<Cell>& map, GlobalWorldObjects& globals, MapConfig& config, const std::vector<sf::Vector2f>& voronoi_points)
{
    // Generate continents
	globals.continents.reserve(config.npeaks);
	for (int i = 0; i < config.npeaks; i++)
	{
		globals.continents.emplace_back(Continent(i));
	}
    // 
    std::vector<std::vector<std::size_t>> active;
    active.reserve(config.npeaks);
    std::vector<bool> assigned(map.size(), false); 
    std::size_t size = map.size(); 
    std::size_t assigned_cells = 0; 

    std::size_t index = 0;
    // Assign the first cells to the continents
    for (int i = 0; i < config.npeaks; i++)
    {
        // Check if the index is already assigned
        while (assigned[index]) {
            index = rand_long() % map.size(); // Pick a new random index
        }
        active.push_back(std::vector<std::size_t>()); // Add a new queue for the continent
        
        globals.continents[i].addCell(index);
        map[index].continent = i;

        assigned[index] = true; 
        assigned_cells++; 
        for (int j = 0; j < map[index].neighbors.size(); j++)
        {
            if (assigned[map[index].neighbors[j]] == false)
            {
                active[i].push_back(map[index].neighbors[j]); 
                assigned[map[index].neighbors[j]] = true;
                assigned_cells++;
            }
        }
    }
    // Assign the rest of the cells to the continents
    while (assigned_cells < size) 
    {
        std::size_t i = rand_long() % config.npeaks; 
        if (active[i].empty()) { continue; } 
        std::size_t index = 0; 
        index = pop_random_i(active[i]);

        globals.continents[i].addCell(index);
        map[index].continent = i;

        for (int j = 0; j < map[index].neighbors.size(); j++) 
        {
            if (assigned[map[index].neighbors[j]] == false) 
            { 
                active[i].push_back(map[index].neighbors[j]); 
                assigned[map[index].neighbors[j]] = true; 
                assigned_cells++; 
            }
        }
    }

    // Direction for continental drift
    for (int i = 0; i < config.npeaks; i++)
    {
        globals.continents[i].setDirection(sf::Vector2f(RandomBetween(-1.0, 1.0), RandomBetween(-1.0, 1.0)));
        globals.continents[i].setAge(RandomBetween(0.5f, 1.0f));
        globals.continents[i].finishContinent(voronoi_points, map);
    }

	// Assign any straglers to the nearest continent
	for (std::size_t i = 0; i < map.size(); i++)
	{
		if (map[i].continent == -1)
		{
			float min_dist = std::numeric_limits<float>::max();
			int min_index = -1;
			for (int j = 0; j < config.npeaks; j++)
			{
				float dist = distance(voronoi_points[i], globals.continents[j].getCenter());
				if (dist < min_dist)
				{
					min_dist = dist;
					min_index = j;
				}
			}
			map[i].continent = min_index;
			globals.continents[min_index].addCell(i);
		}
	}

	// Calculate the height of the continents
	for (int i = 0; i < config.npeaks; i++) 
	{
		float height = RandomBetween(0.1, 0.9); 
		globals.continents[i].setHeight(height); 
	}

    // ensure that at least 1/3 of the continents are above 0.5
    int count = 0;
    for (int i = 0; i < config.npeaks; i++) 
    {
        if (globals.continents[i].getHeight() > 0.5) { count++; }
    }
    if (count < config.npeaks / 4)
    {
        for (int i = 0; i < static_cast<int>(config.npeaks / 4); i++)
        {
            if (globals.continents[i].getHeight() < 0.5) { globals.continents[i].setHeight(normalDistPDF(0.5f,0.2f)); }
        }
    }

}

void continent_interaction(std::vector<Cell>& map, GlobalWorldObjects& globals, int mapGenMode = 3)
{
    /// TODO: There is a problem with directions not leading to correct height additions..
	// Get the interactions between the continents
    /*
    1. Determine boundry cells
    2. Get direction similarity to neighbors and add height
    3. Propagate
    */

    for (std::size_t cellIndex = 0; cellIndex < map.size(); cellIndex++)
    {
        Cell& cell = map[cellIndex];

        bool isBoundry = false;
        std::vector<int> neighboringContinents;

        for (int neighborIdx : cell.neighbors)
        {
            if (neighborIdx >= 0 && neighborIdx < map.size())
            {
                int neighborContinent = map[neighborIdx].continent;
                if (neighborContinent != cell.continent 
                    && std::find(neighboringContinents.begin(), neighboringContinents.end(), neighborContinent) == neighboringContinents.end())
                {
                    neighboringContinents.push_back(neighborContinent);
                    isBoundry = true;
                }
            }
        }
        if (!isBoundry)
        {
            if (mapGenMode == 3)
            {
                cell.height = globals.continents[cell.continent].getHeight();
                continue; // If not a boundry
            }            
        }

        float heightIncrease = 0.0f;
        bool volcanicActivity = false;
        bool isRift = false;
        bool isTransformBoundry = false;
        if (isBoundry)
        {
            // add to the boundry list of the continent
            globals.continents[cell.continent].addBoundryCell(cellIndex);

            sf::Vector2f mainDirection = globals.continents[cell.continent].getDirection();
            float mainHeight = globals.continents[cell.continent].getHeight();
            if (mapGenMode != 3) { mainHeight = map[cellIndex].height; } // Use the cell height instead of the continent height  }

            bool isMainOcean = mainHeight < globals.seaLevel;

            for (int neighborContinent : neighboringContinents)
            {
                sf::Vector2f neighborDirection = globals.continents[neighborContinent].getDirection();
                float neighborHeight = globals.continents[neighborContinent].getHeight();
                if (mapGenMode != 3) { mainHeight = map[cellIndex].height; }
                bool isNeighborOcean = neighborHeight < globals.seaLevel;

                // Calculate dotproduct for Direction similarity (also normalize)
                float mainMagnitude = std::sqrt(mainDirection.x * mainDirection.x + mainDirection.y * mainDirection.y);  
                float neighborMagnitude = std::sqrt(neighborDirection.x * neighborDirection.x + neighborDirection.y * neighborDirection.y);  
                
                if (mainMagnitude > 0.001f && neighborMagnitude > 0.001f) 
                {
                    sf::Vector2f mainNormalized(mainDirection.x / mainMagnitude, mainDirection.y / mainMagnitude); 
                    sf::Vector2f neighborNormalized(neighborDirection.x / neighborMagnitude, neighborDirection.y / neighborMagnitude); 

                    // Calculate dot product
                    float dotProduct = mainNormalized.x * neighborNormalized.x + mainNormalized.y * neighborNormalized.y; 

                    // (-1,1) : -1 is directly diverging and 1 parralel and 0 means converging
                    if (dotProduct < -0.7f)  // Transform fault boundaries (parallel but opposite directions)
                    { 
                        isTransformBoundry = true;
                        heightIncrease += RandomBetween(-0.05f, 0.05f);
                        if (RandomBetween(0.0f, 1.0f) < 0.1f) {
                            volcanicActivity = true;
                        }
                    }
                    else if (std::abs(dotProduct) > 0.25f) // Convergent or divergent boundries
                    {

                        bool isConverging = dotProduct < 0;  // Negative dot product suggests opposite directions
                        
                        float impactFactor = isConverging ? -dotProduct : dotProduct * 0.5;  // Less impact for diverging

                        if (isMainOcean != isNeighborOcean)  // Ocean-continent boundary
                        {
                            float baseIncrease = 0.1f * impactFactor;

                            // Oceanic plate subducts under terrestrial
                            if (isMainOcean)
                            {
                                heightIncrease += baseIncrease * 0.5f;

                                if (RandomBetween(0.0f, 1.0f) < 0.7f * std::abs(dotProduct))
                                {
                                    volcanicActivity = true;
                                }
                            }
                            else
                            {
                                // Neighbor is oceanic and subducts
                                heightIncrease += baseIncrease * 1.0f;
                                if (RandomBetween(0.0f, 1.0f) < 0.85f * std::abs(dotProduct))
                                {
                                    volcanicActivity = true;
                                }
                            }
                        }
                        else if (!isMainOcean && !isNeighborOcean) // Continent continent
                        {
                            float plateHeightFactor = (mainHeight + neighborHeight) / 2.0f;
                            heightIncrease += impactFactor * 0.2f * plateHeightFactor; // adjust here! the 0.4f
                            // Some volcanic activity in continental collisions
                            if (RandomBetween(0.0f, 1.0f) < 0.4f * std::abs(dotProduct)) {
                                volcanicActivity = true;
                            }
                        }
                        else // Ocean-ocean boundary
                        {
                            // Oceanic collision: island arcs, high volcanic activity
                            heightIncrease += dotProduct * 0.25f; // Less dramatic height increase

                            // Very high chance of volcanic activity
                            if (RandomBetween(0.0f, 1.0f) < 0.9f * std::abs(dotProduct)) {
                                volcanicActivity = true;
                            }
                        }                        
                    }
                    else {
                        // Plates moving in approximately perpendicular directions
                        // Minor deformation
                        heightIncrease += RandomBetween(-0.02f, 0.02f);
                    }
                }
            }
        }
        // Change based on age of continent
        heightIncrease *= globals.continents[cell.continent].getAge();

        // Height increase:
		float baseHeight = globals.continents[cell.continent].getHeight();
        if (mapGenMode != 3) { baseHeight = map[cellIndex].height; }
        cell.height = std::min(1.0f, baseHeight + heightIncrease); 

		if (volcanicActivity) {
			cell.volcanicActivity = true;
            cell.height = std::min(1.0f, cell.height + RandomBetween(0.1f, 0.2f));
		}

        // Propagation!
        const int propagationDepth = std::floorf(2 + 5 * globals.continents[cell.continent].getAge()); // How far to spread the mountain effect
        std::vector<bool> visited(map.size(), false);
        visited[cellIndex] = true;

        Queue<std::pair<int, int>> propagationQ;
        propagationQ.push({ cellIndex, 0 });

        while (!propagationQ.empty())
        {
            auto [currentIndex, distance] = propagationQ.pop_front();

            if (distance >= propagationDepth)
            {
                continue;
            }
            
            float distanceFactor = 1.0f - static_cast<float>(distance) / propagationDepth;
            float propagationIncrease = 0.0f;

            if (isRift)
            {
                propagationIncrease = heightIncrease * distanceFactor * 0.4f;
            }
            else if (isTransformBoundry)
            {
                propagationIncrease = heightIncrease * distanceFactor * 0.3f;
            }
            else {
				propagationIncrease = heightIncrease * distanceFactor * 0.6f;
            }

            // Add neighbors
            for (std::size_t neighborIdx : map[currentIndex].neighbors)
            {
                if (!visited[neighborIdx] && neighborIdx >= 0 
                    && neighborIdx < map.size() 
                    && map[neighborIdx].continent == cell.continent)
                {
					visited[neighborIdx] = true;
					map[neighborIdx].height = std::min(1.0f, map[neighborIdx].height + propagationIncrease);

                    // Propagate Volcanic activity
                    if (volcanicActivity && RandomBetween(0.0f, 1.0f) < 0.2f * distanceFactor)
                    {
						map[neighborIdx].volcanicActivity = true;
                        map[neighborIdx].height = std::min(1.0f, map[neighborIdx].height + RandomBetween(0.05f, 0.1f) * distanceFactor);
                    }

					propagationQ.push({ neighborIdx, distance + 1 });
				}
			}
        }
    }
}

void simplex_noise_continent(std::vector<Cell>& map, 
    GlobalWorldObjects& globals, 
    const MapConfig& config,
    const std::vector<sf::Vector2f>& positons)
{
    // Generate a layer of simplex noise and add it to the height
    int seed = rand();
    SimplexNoise simplex(seed);

	// Find Bounds
    float minX = std::numeric_limits<float>::max(); 
    float maxX = std::numeric_limits<float>::min(); 
    float minY = std::numeric_limits<float>::max(); 
    float maxY = std::numeric_limits<float>::min();

    for (const auto& pos : positons) { 
        minX = std::min(minX, pos.x);   
        maxX = std::max(maxX, pos.x);  
        minY = std::min(minY, pos.y);   
        maxY = std::max(maxY, pos.y);  
    }

    // Configure noise parameters (SHOULD BE IN CONFIG)
    const int octaves = 3;         // Number of frequency layers (more = more detail)
    const float persistence = 0.5f; // How much each octave contributes (0-1)
    const float noiseScale = 0.09f; // Scale of the noise (smaller = larger features)
    const float noiseStrength = 0.3f; // How much the noise affects the height (0-1)
	
    // Apply noise to all cells
    for (std::size_t i = 0; i < map.size(); i++)
    {
        float normalizedX = (positons[i].x - minX) / (maxX - minX);
        float normalizedY = (positons[i].y - minY) / (maxY - minY);

        float noiseValue = simplex.octaveNoise(
            normalizedX / noiseScale,
            normalizedY / noiseScale,
            octaves,
            persistence
        );

        // convert noise to [0,1]
        noiseValue = (noiseValue + 1.0f) * 0.5f;

        // Apply noise with strength factor and preserve existing height features
        float originalHeight = map[i].height;
        // Different noise application based on terrain type
        if (originalHeight < 0.45f) {
            // Ocean floor - gentle noise
            map[i].height += noiseValue * noiseStrength * 0.5f - (noiseStrength * 0.25f);
        }
        else if (originalHeight > 0.7f) {
            // Mountains - stronger noise to create peaks and valleys
            map[i].height += noiseValue * noiseStrength * 1.2f - (noiseStrength * 0.6f);
        }
        else {
            // Regular terrain - normal noise
            map[i].height += noiseValue * noiseStrength - (noiseStrength * 0.5f);
        }
        // Clamp height to valid range
        map[i].height = std::max(0.0f, std::min(1.0f, map[i].height));
    }

}

// MEthod should be enum or something
// 1 = k-peaks, 2 = k-peaks continet, 3 = Continental
// k-point smooth height generator
// there is a max of RAND_MAX_LONG (about a million cells)
void random_height_gen(std::vector<Cell>& map,
    GlobalWorldObjects& globals, MapConfig config, 
    const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voronoi_points)
{   
    /// TODO: Continent direction does not translate to the correct height add (it needs to use velocity??)
    if (config.height_method < 3)
    {
        // Method 1 is without continent interactions and 2 is with.
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
        std::vector<std::size_t> active;
        active.reserve(map.size() - 1);

        for (int i = 0; i < config.npeaks; i++)
        {
            std::size_t index = rand_long() % map.size();
            map[index].height = RandomBetween(0.8, 1.0);
            active.insert(std::end(active), std::begin(map[index].neighbors), std::end(map[index].neighbors));

            // Continents
            globals.continents.emplace_back(Continent(i));
            globals.continents[i].addCell(index);
            map[index].continent = i;
			globals.continents[i].setCenter(points[index]); // Set the center of the continent to the first cell added

            // add the actually inserted neighbors into continent
            for (std::size_t neighbor = 0; neighbor < map[index].neighbors.size(); neighbor++)
            {
				globals.continents[i].addCell(map[index].neighbors[neighbor]);
				map[map[index].neighbors[neighbor]].continent = i;
            }
        }

        while (active.empty() == false)
        {
            std::size_t index = pop_random_i(active);

            float height_sum = 0.0;
            int count_values = 0;
            for (std::size_t j = 0; j < map[index].neighbors.size(); j++)
            {
                // small probability of random height increase, THIS is heavily up to tuning for interesting maps
                // Also should be reconsidered
                if (RandomBetween(0.0, 1.0) < config.prob_of_island && height_sum < config.dist_from_mainland && count_values > 1)
                {
                    map[map[index].neighbors[j]].height = RandomBetween(0.6, 0.9);
                    active.insert(std::begin(active), std::begin(map[map[index].neighbors[j]].neighbors), std::end(map[map[index].neighbors[j]].neighbors));
					
					for (std::size_t k = 0; k < map[map[index].neighbors[j]].neighbors.size(); k++)
					{
                        globals.continents[map[index].continent].addCell(map[map[index].neighbors[j]].neighbors[k]);
                        map[map[map[index].neighbors[j]].neighbors[k]].continent = map[index].continent;
                    }
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
                    //insert_unique(active, map[index].neighbors[j]);
                    if (std::find(active.begin(), active.end(), map[index].neighbors[j]) == active.end()) {
                        active.push_back(map[index].neighbors[j]);
                        // Add the neighbors to the continent as well
                        globals.continents[map[index].continent].addCell(map[index].neighbors[j]);
						map[map[index].neighbors[j]].continent = map[index].continent;
                    }
                }
            }
            if (count_values == 0) { active.push_back(map[index].id); }
            else {
                map[index].height = clamp((height_sum / count_values) + RandomBetween(-config.delta_max_neg, config.delta_max_pos), 1.0, 0.0);
            }
        }
        // Fill out missing continent values 
		for (std::size_t i = 0; i < map.size(); i++)
		{
			if (map[i].continent == -1)
			{
                // check neighbors and find the first non missing
				for (std::size_t j = 0; j < map[i].neighbors.size(); j++)
                {
                    if (map[map[i].neighbors[j]].continent != -1)
                    {
						map[i].continent = map[map[i].neighbors[j]].continent;
						break;
					}
				}
            }
		}
        for (std::size_t i = 0; i < globals.continents.size(); i++)
        {
            std::vector<std::size_t> cells = globals.continents[i].getCells();
            double sum_height = 0;
            for (auto cell : cells)
			{
				if (cell > map.size()) { throw std::out_of_range("Cell index out of range"); }

                sum_height += map[cell].height;
            }
			globals.continents[i].setHeight(sum_height / static_cast<double>(cells.size()));

            globals.continents[i].setAge(RandomBetween(0.5, 0.9));
			globals.continents[i].setDirection(sf::Vector2f(RandomBetween(-1.0, 1.0), RandomBetween(-1.0, 1.0)));
        }

		if (config.height_method == 2)
		{
            continent_interaction(map, globals, config.height_method);
        }
        
        rise(map); // calculate the rise of the map with the new height values
    }
    else
    {
        // Method 3 is a continental generation method
        /*
		1. Initiate K queues for the continents
        2. Initiate a list of cells for each continent
		3. Pick K random starting cells
		4. Randomly pick a continent and add the neighbors to the active queue
		5. Repeat until all cells are assigned
        6. Assign a height to each continent then add noise filter (simplex?) and add to each cell in the continent
		7. Assign directions for continental drift
		8. Calculate the interactions between the continents
		9. Calculate the rise of the map with the new height values
        */

        continent_generation(map, globals, config, voronoi_points);

        // Calculate the interactions between the continents
        continent_interaction(map, globals);

        rise(map); // calculate the rise of the map with the new height values 

		smooth_height(map, config.rise_threshold, 1, config.smooth_method); // smooth the height values

        // Overlay a noise filter (simplex noise)
        simplex_noise_continent(map, globals, config, points);

        for (int i = 0; i < globals.continents.size(); i++)
        {
            globals.continents[i].generateBoundryLine(map, voronoi_points);
        }
		
		rise(map); // calculate the rise of the map with the new height values
    }
    
}


void noise_height(std::vector<Cell>& map, 
    GlobalWorldObjects& globals,
    const MapConfig& config,
    const std::vector<sf::Vector2f>& points, 
    int iterations)
{
    if (config.heigth_noise_method == 1)
    {
        size_t size = map.size();
        for (int i = 0; i < iterations; i++)
        {
            for (size_t j = 0; j < size; j++)
            {
                map[j].height = map[j].height + RandomBetween(-0.005, 0.005);
            }
        }
    }
    else if (config.heigth_noise_method == 2)
    {
		simplex_noise_continent(map, globals, config, points);
    }
    else
    {
		throw std::invalid_argument("Invalid method for noise height generation");
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

void riverIteration(std::vector<Cell>& map, GlobalWorldObjects& globals, std::vector<std::size_t>& stack, std::size_t start, std::size_t river_id)
{
    // Recursive River function
	// 1. Get the height difference between the start cell and its neighbors
	// 2. Sort the neighbors based on the height difference
	// 3. If the height difference is negative, the river ends or a lake is generated
	// 4. If the height difference is positive, the river continues to the neighbor with the lowest height difference
	// 5. If the neighbor is snow, lake or ocean, the river ends
	// 6. If the neighbor is a river, pick a neighbor that has less than 2 river neighbors
	// 7. If no neighbor is found, the river ends
	// 8. If a neighbor is found, the river continues to the neighbor and the function is called recursively

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
				// check neighbors if they have a lake id add this cell to the lake object otherwise create a new lake object
                // TODO: LAKES
				for (int j = 0; j < map[map[start].neighbors[i]].neighbors.size(); j++)
                {
                    if (map[map[map[start].neighbors[i]].neighbors[j]].lakeBool == true)
                    {
						globals.lakes[map[map[map[start].neighbors[i]].neighbors[j]].lakeId].addCell(map[start].neighbors[i]);
						map[map[start].neighbors[i]].lakeId = map[map[map[start].neighbors[i]].neighbors[j]].lakeId;
						break;
                    }
                    else
					{
						int lake_id = globals.lakes.size();
						globals.lakes.emplace_back(Lake(lake_id));
						globals.lakes[globals.lakes.size() - 1].addCell(map[start].neighbors[i]);
                        map[map[start].neighbors[i]].lakeId = lake_id;
					}
                }
            }
        }
    }
    else
    {
        //if (map[HeightDiff[HeightDiff.size() - order].first].snowBool == true){ return;}else
        // We check if the neighbor is snow, ocean or river
        if (map[HeightDiff[HeightDiff.size() - order].first].oceanBool == true)
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
				// If no neighbor is found the river feeds into any neighbor that is an ocean, snow or river
                globals.rivers[river_id].addCell(HeightDiff[HeightDiff.size() - order].first);
				globals.rivers[river_id].setParentRiver(map[HeightDiff[HeightDiff.size() - order].first].riverId);
				globals.rivers[map[HeightDiff[HeightDiff.size() - order].first].riverId].addTributary(river_id);
                return;
            }
        }
    }

	// is this ever reached?? TODO: Check if this is ever reached
    map[HeightDiff[HeightDiff.size() - order].first].riverBool = true;
    globals.riverCells.push_back(HeightDiff[HeightDiff.size() - order].first);
    map[HeightDiff[HeightDiff.size() - order].first].riverStr = map[start].riverStr; //- RandomBetween(0.001, 0.003);
    globals.rivers[river_id].addCell(HeightDiff[HeightDiff.size() - order].first);
	map[HeightDiff[HeightDiff.size() - order].first].riverId = river_id;
    riverIteration(map, globals, stack, HeightDiff[HeightDiff.size() - order].first, river_id);

    // FLow strenght calculate here

}

void calcRiverStart(std::vector<Cell>& map, GlobalWorldObjects& globals, const std::vector<sf::Vector2f>& points, const std::vector<sf::Vector2f>& voronoi_points)
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

    // Lake id start
	int newest_lake_id = 0;

    // Start the river from the snow cells
    std::size_t i = 0;
    while (!stack.empty())
    {
        // TODO: LAKES
        // ADD: After each iteration check all neighbors of all lakes and see if they need to be rivers, and add them to the stack. (at random!)
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

		// Check any new lakes that might have been created and add neighbors to the stack
		for (int j = newest_lake_id; j < globals.lakes.size(); j++)
        {
            // Finish any new lake
			globals.lakes[j].finishLake(map, voronoi_points);

			// Add neighbors to the stack
            for (int j = 0; j < globals.lakes[newest_lake_id].getCells().size(); j++)
            {
				std::vector<int> neighbors = map[globals.lakes[newest_lake_id].getCells()[j]].neighbors;
                for (int k = 0; k < map[globals.lakes[newest_lake_id].getCells()[j]].neighbors.size(); k++)
                {
					if (map[neighbors[k]].riverBool == false && map[neighbors[k]].lakeBool == false && map[neighbors[k]].oceanBool == false && map[neighbors[k]].snowBool == false)
                    {
						stack.push_back(neighbors[k]);
					}
				}
			}
			// Increment the lake index/id for the next lake
			newest_lake_id++;
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

void calcPercepitation(std::vector<Cell>& map, 
    const std::vector<sf::Vector2f>& points, 
    GlobalWorldObjects& globals, 
    int runs = 3,
    float max_percipitation = 500.0f,
	float ocean_evaporation_factor = 1.0f,
	float land_evapotranspiration_factor = 0.3f,
    float moisture_loss_rate = 0.08f,
	float orographic_factor = 2.5f,
	float condensation_rate = 0.12f
)
{
	// Alogrithm Overview:
	// 1. For each cell, calculate initial moisture based on evaporation (higher over oceans, modulated by temperature and vegetation)
	// 2. For a number of advection steps:
	//    a. For each cell, determine the best neighbor aligned with wind direction
	//    b. Move moisture to that neighbor based on wind strength
	//    c. Calculate precipitation based on moisture, temperature, and terrain features (orographic lift)
	//   d. Reduce moisture by precipitation and natural loss
	// 3. Repeat for multiple runs to simulate seasonal variations
	// 4. Average the precipitation over all runs for final value
	// 5. Clamp and smooth the final precipitation values
	
    for (int run = 0; run < runs; run++)
    {
        std::vector<float> moisture(map.size(), 0.0f);
        std::vector<float> newPrecipitation(map.size(), 0.0f);

        // Step 1: Generate initial moisture from evaporation
        for (std::size_t i = 0; i < map.size(); i++)
        {
            float evaporation = 0.0f;

            if (map[i].oceanBool)
            {
                float tempFactor = clamp((map[i].temp + 20.0f) / 50.0f, 1.0f, 0.1f);
                evaporation = ocean_evaporation_factor * tempFactor * 100.0f;
            }
            else
            {
                float tempFactor = clamp((map[i].temp + 10.0f) / 40.0f, 1.0f, 0.1f);
                float vegetationFactor = map[i].treeBool ? 1.2f : 0.7f;
                evaporation = land_evapotranspiration_factor * tempFactor * vegetationFactor * 25.0f;
            }
            moisture[i] = evaporation;
        }

        // Step 2: Moisture advection with precipitation
        const int advectionSteps = 50;

        // Vary the processing order to avoid bias
		std::vector<std::size_t> cellOrder(map.size());
        for (std::size_t i = 0; i < map.size(); i++) {
            cellOrder[i] = i;
        }
        if (run > 0) {
			std::shuffle(cellOrder.begin(), cellOrder.end(), std::default_random_engine(rand()));
        }
                
        for (int step = 0; step < advectionSteps; step++)
        {
            std::vector<float> newMoisture(map.size(), 0.0f);
            
            for (std::size_t idx = 0; idx < cellOrder.size(); idx++)
            {
				std::size_t i = cellOrder[idx];

                if (moisture[i] < 0.01f) continue;

                float cellMoisture = moisture[i];
                float windDir = radians(map[i].windDir);
                float windStr = map[i].windStr;

                // Add slight directional variance per run to simulate seasonal wind shifts
				float windVariance = std::sin(run * 2.0f) * 20.f; // +/- 20 degrees
				windDir += radians(windVariance);

                // Find best neighbor aligned with wind
                int bestNeighbor = -1;
                float bestAlignment = -1.0f;
                
                for (int neighborIdx : map[i].neighbors)
                {
                    if (neighborIdx < 0 || neighborIdx >= map.size()) continue;
                    
                    float dx = points[neighborIdx].x - points[i].x;
                    float dy = points[neighborIdx].y - points[i].y;
                    float neighborDir = std::atan2(dy, dx);

                    float angleDiff = std::abs(neighborDir - windDir);
                    if (angleDiff > PI) angleDiff = 2.0f * PI - angleDiff;
                    
                    float alignment = std::cos(angleDiff);
                    
                    if (alignment > bestAlignment)
                    {
                        bestAlignment = alignment;
                        bestNeighbor = neighborIdx;
                    }
                }

                // Calculate precipitation
                float precipitationAmount = 0.0f;

                // 1. Baseline natural condensation (happens everywhere with sufficient moisture)
                if (cellMoisture > 10.0f)  // Only if there's enough moisture
                {
                    float moistureFactor = std::min(cellMoisture / 100.0f, 1.0f);
                    float baseCondensation = condensation_rate * moistureFactor * 12.0f;

					// Vary ocndensation rate per run to simulate seasonal changes
					float seasonalFactor = 0.8f + 0.4f * (run / (float)runs);

                    precipitationAmount += baseCondensation * seasonalFactor;
                }

                // 2. Temperature-driven condensation (colder air holds less moisture)
                if (map[i].temp < 20.0f && cellMoisture > 20.0f)
                {
                    float coldFactor = (20.0f - map[i].temp) / 30.0f;  // Stronger at colder temps
                    coldFactor = clamp(coldFactor, 1.0f, 0.0f);
                    float coldPrecip = cellMoisture * 0.08f * coldFactor;
                    precipitationAmount += coldPrecip;
                }

                // 3. Orographic precipitation (mountains force uplift)
                if (bestNeighbor >= 0 && !map[i].oceanBool)  // Only over land
                {
                    float heightDiff = map[bestNeighbor].height - map[i].height;
                    if (heightDiff > 0.015f)  // Lower threshold for gentler slopes
                    {
                        float liftFactor = std::tanh(orographic_factor * heightDiff);
                        float orographicPrecip = cellMoisture * liftFactor * 0.35f;
                        precipitationAmount += orographicPrecip;
                    }
                }

                // 4. Oceanic storm precipitation (based on moisture and wind)
                if (map[i].oceanBool && cellMoisture > 50.0f && windStr > 0.6f)
                {
                    float stormFactor = (windStr - 0.6f) * 2.5f;  // Stronger winds = more storms
                    float oceanStormPrecip = cellMoisture * 0.06f * stormFactor;
                    precipitationAmount += oceanStormPrecip;
                }

                // Cap precipitation to avoid depleting all moisture
                precipitationAmount = std::min(precipitationAmount, cellMoisture * 0.65f);

                // Add precipitation
                newPrecipitation[i] += precipitationAmount;

                // Remaining moisture after precipitation
                float remainingMoisture = cellMoisture - precipitationAmount;

                // Transfer moisture to downwind neighbor
                if (bestNeighbor >= 0 && bestAlignment > 0.0f)
                {
                    // Transfer efficiency based on wind and alignment
					// Vary transfer factor per run to simulate seasonal wind strength changes
                    float runVariation = 0.85 + 0.1f * std::cos(run * 1.5f);
                    float transferFactor = windStr * std::pow(std::max(0.0f, bestAlignment), 0.4f) * runVariation;
                    float transferredMoisture = remainingMoisture * transferFactor;

                    // Rain shadow effect (descending terrain)
                    if (!map[i].oceanBool)  // Only apply to land
                    {
                        float heightDiff = map[bestNeighbor].height - map[i].height;
                        if (heightDiff < -0.02f)
                        {
                            transferredMoisture *= 0.70f;  // Rain shadow dries air
                        }
                    }

                    // Natural moisture loss during transport
                    transferredMoisture *= (1.0f - moisture_loss_rate);

                    newMoisture[bestNeighbor] += transferredMoisture;
                    newMoisture[i] += remainingMoisture * (1.0f - transferFactor);
                }
                else
                {
                    // No good neighbor - moisture either precipitates or dissipates
                    newPrecipitation[i] += remainingMoisture * 0.25f;
                    newMoisture[i] += remainingMoisture * 0.75f;
                }
            }
            
            moisture = newMoisture;
        }

        // Step 3: Accumulate precipitation across runs
        for (std::size_t i = 0; i < map.size(); i++)
        {
            if (run == 0)
            {
                map[i].percepitation = newPrecipitation[i];
            }
            else
            {
                // Weight each run equally
                map[i].percepitation = (map[i].percepitation * run + newPrecipitation[i]) / (run + 1.0f);
            }
        }
    }
	// Step 4: Add tiny simplex noise for variability (needs a look)
	SimplexNoise simplexNoise = SimplexNoise(rand());
    for (std::size_t i = 0; i < map.size(); i++)
    {
		float noise = simplexNoise.noise(points[i].x * 0.02f, points[i].y * 0.02f) * 2.0f; // small variation
        map[i].percepitation += noise;
        map[i].percepitation = std::max(0.0f, map[i].percepitation); // ensure non-negative
	}


	// Step 5: Cap the precipitation values
    for (std::size_t i = 0; i < map.size(); i++)
    {
        map[i].percepitation = std::min(map[i].percepitation, max_percipitation);
	}

    // Step 6: Smooth precipitation for gradual transitions
    std::vector<float> smoothedPrecip(map.size());
    for (std::size_t i = 0; i < map.size(); i++)
    {
        float sum = map[i].percepitation * 2.5f;  // Weight center
        int count = 2.5f;

        for (int neighborIdx : map[i].neighbors)
        {
            sum += map[neighborIdx].percepitation;
            count++;
        }

        smoothedPrecip[i] = sum / count;
    }

    for (std::size_t i = 0; i < map.size(); i++)
    {
        map[i].percepitation = smoothedPrecip[i];
    }
}

void calcHumid(std::vector<Cell>& map)
{
    // Humidity should depend on:
    // 1. Temperature (warmer air holds more moisture)
    // 2. Precipitation (more rain = more moisture available)
    // 3. Proximity to water bodies
    // 4. Evapotranspiration from vegetation
    for (int i = 0; i < map.size(); i++)
    {
        float humidity = 0.0f;

        if (map[i].oceanBool)
        {
			// Ocean cells: very high humidity
            // Warmer water = more evaporation
			float tempFactor = clamp((map[i].temp + 10.0f) / 40.0f, 1.0f, 0.1f);
			humidity = 0.8 + 0.2f * tempFactor;
        }
        else
        {
            // Base humidity from precipitation and temperature
            // Logarithmic realtionship: more precip = higher humidity, but with diminishing returns
			float precipFactor = std::log(map[i].percepitation + 1.0f) / std::log(500.0f + 1.0f); // assuming 500mm/year max
			precipFactor = clamp(precipFactor, 1.0f, 0.0f);
            
            // Temperature effect on stauration capacity
			// warmer air holds more moisture, but also increases evapotranspiration
			float tempFactor = 1.0f / (1.0f + std::exp(-0.1f * (map[i].temp - 15.0f))); // sigmoid centered at 15C

			// Distance to ocean effect, coastal areas are more humid
			float oceanProximityFactor = std::exp(-0.03f * map[i].distToOcean); // decays with distance

            // Vegetation/surface moisture effect (simplified, should link to biome later)
			float surfaceMoisture = map[i].treeBool ? 0.1f : 0.0f; // more vegetation = more evapotranspiration

			// Combine factors
            humidity = precipFactor * 0.5f // 50% for precipitation
                     + tempFactor * 0.3f      // 30% for temperature
                     + oceanProximityFactor * 0.15f // 15% for ocean proximity
				+ surfaceMoisture * 0.05f; // 5% for surface moisture

			// Height adjustment: higher altitudes tend to be less humid
            if (map[i].height > 0.7f)
            {
				humidity *= (1.0f - (map[i].height - 0.7f) * 0.5f); // reduce humidity at high altitudes
            }
			humidity = clamp(humidity, 1.0f, 0.0f);
        }
		map[i].humidity = humidity;
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
	// Wind is using simplex noise to generate variation along the convergence lines

    // Generate noise for wind variation
    int windSpeed = rand();
	SimplexNoise noiseWind(windSpeed);

	// Find map bounds for normalization
	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::min();

    for (const auto& pos : points)
    {
        minX = std::min(minX, pos.x);
        maxX = std::max(maxX, pos.x);
        minY = std::min(minY, pos.y);
        maxY = std::max(maxY, pos.y);
	}
    // Wind parameters
	const float noiseScale = 0.15f;      // Scale of the noise (smaller = larger features)
	const float noiseAmplitude = 45.0f;  // Amplitude of the noise in degrees
	const int octaves = 2;               // detail levels
	const float persistence = 0.6f;      // amplitude decrease per octave

    // Calculate base wind patterns from latitude zones
	std::vector<float> convergenceY = scalarMultiplication(globals.convergenceLines, (float)MAXHEIGHT);

    for (size_t i = 0; i < map.size(); i++)
    {
		float normalizedX = (points[map[i].id].x - minX) / (maxX - minX);
		float normalizedY = (points[map[i].id].y - minY) / (maxY - minY);

		// Find closest convergence line
        int closestLine = 0;
        float min_dist = 1000000.f;
        for (int j = 0; j < convergenceY.size(); j++)
        {
            float dist = points[map[i].id].y - convergenceY[j];
            if (abs(dist) < min_dist)
            {
                min_dist = abs(dist);
                closestLine = j;
            }
		}

        // Base wind from zone
		float baseDir = globals.windDirection[closestLine];
		float baseStr = globals.windStrength[closestLine];

		// Distance from convergence line affects strength
		float distFromLine = min_dist / (MAXHEIGHT / (float)globals.convergenceLines.size());
		distFromLine = clamp(distFromLine, 1.f, 0.f);

        // strength varies with from zone center
		float strengthModifier = 0.7f + 0.3f * (1.0f - distFromLine);

        // Generate noise variation for direction
        float noiseValue = noiseWind.octaveNoise(
            normalizedX / noiseScale,
            normalizedY / noiseScale,
            octaves,
            persistence
        );

		// Apply noise to create natural wind variation
		float windDir = noiseValue * noiseAmplitude;

        // Coriolis-like effect: wind curves near poles
		float latitudeFactor = std::abs(normalizedY - 0.5f) * 2.0f; // 0 at equator, 1 at poles
        float coriolisDeflection = latitudeFactor * 15.0f * (normalizedY > 0.5f ? 1.0f : -1.0f);

        // Combining direction components
		float finalDir = normalizeAngle(baseDir + windDir + coriolisDeflection);

		// Terrain effects on wind strength (mountains block wind) (can be improved)
        float terrainFactor = 1.0f;
        if (map[i].height > 0.75f)
        {
            terrainFactor = 1.0f - (map[i].height -0.6f) * 0.7f; // reduce strength in high terrain
		}
		// Ocean vs land: ocean has stronger more consistent winds
		float surfaceFactor = map[i].oceanBool ? 1.1f : 0.9f;

		// Calculate final wind strength
		float finalStr = baseStr * strengthModifier * terrainFactor * surfaceFactor;
        finalStr = clamp(finalStr + RandomBetween(-0.1f, 0.1f), 1.f, 0.f);

		map[i].windDir = finalDir;
		map[i].windStr = finalStr;
    }

	// Smoothing winds to create coherent patterns
	// Average with neighbors
	std::vector<float> smoothedDirs(map.size(), 0.f);
	std::vector<float> smoothedStrs(map.size(), 0.f);

    for (size_t i = 0; i < map.size(); i++)
    {
		float sumX = 0.f;
		float sumY = 0.f;
        float sumStr = 0.f;
		int count = 0;

		float selfRad = radians(map[i].windDir);
		sumX += std::cos(selfRad) * 2.0f;
        sumY += std::sin(selfRad) * 2.0f;
        sumStr += map[i].windStr * 2.0f;
		count += 2;
        for (int neighborIdx : map[i].neighbors)
        {
			float neighborRad = radians(map[neighborIdx].windDir);
            sumX += std::cos(neighborRad);
            sumY += std::sin(neighborRad);
			sumStr += map[neighborIdx].windStr;
            count++;
        }
		float avgRad = std::atan2f(sumY / count, sumX / count);
		smoothedDirs[i] = normalizeAngle(avgRad * 180.0f / PI);
		smoothedStrs[i] = sumStr / count;
    }
	// Apply smoothed values
    for (size_t i = 0; i < map.size(); i++)
    {
        map[i].windDir = smoothedDirs[i];
        map[i].windStr = smoothedStrs[i];
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

