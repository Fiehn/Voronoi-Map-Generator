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
#include "BiomeGen.hpp"
#include <SFML/System/Vector2.hpp>
#include "util.hpp"
#include "PlanetGen.hpp" // The steps that only walk the cell graph live there now


//////////
// Continet Generation
//////////
BoundaryType determineBoundaryType(const Continent& continentA, 
    const Continent& continentB, float dotProduct)
{
    bool converging = dotProduct < -0.2f;
	bool diverging = dotProduct > 0.6f;
	bool transform = std::abs(dotProduct + 0.7f) < 0.3f;
    if (transform) {
		return BoundaryType::Transform;
    }
    if (converging) {
        // Both continental = Himalayan collision
        if (continentA.plateType == PlateType::Continental &&
            continentB.plateType == PlateType::Continental) {
            return BoundaryType::ContinentalCollision;
        }

        // Mixed plate interactions
        if (continentA.plateType == PlateType::Mixed ||
            continentB.plateType == PlateType::Mixed) {
            // Mixed plates generally subduct like oceanic due to higher density
            return BoundaryType::Subduction;
        }

        // One oceanic, one continental (or at least one oceanic)
        return BoundaryType::Subduction;
    }

    if (diverging) {
        // Both oceanic = mid-ocean ridge
        if (continentA.plateType == PlateType::Oceanic &&
            continentB.plateType == PlateType::Oceanic) {
            return BoundaryType::OceanicRift;
        }

        // Mixed plate rifting creates island chains
        if (continentA.plateType == PlateType::Mixed ||
            continentB.plateType == PlateType::Mixed) {
            return BoundaryType::OceanicRift;
        }

        // Continental rifting
        if (continentA.plateType == PlateType::Continental &&
            continentB.plateType == PlateType::Continental) {
            return BoundaryType::ContinentalRift;
        }

        // Default to oceanic rift for mixed cases
        return BoundaryType::OceanicRift;
    }

    return BoundaryType::None;
}
void continent_generation(std::vector<Cell>& map, GlobalWorldObjects& globals, MapConfig& config, const std::vector<sf::Vector2f>& voronoi_points)
{
	// reserve space for continents
	globals.continents.reserve(config.npeaks);
    for (int i = 0; i < config.npeaks; i++)
    {
        globals.continents.emplace_back(Continent(i));
    }
	// Prepare active queues for continent assignment
	std::vector<std::vector<std::size_t>> active(config.npeaks);
    std::vector<bool> assigned(map.size(), false);
	std::size_t size = map.size();
    std::size_t assigned_cells = 0;

	// Assign seed cells to each continent
    for (int i = 0; i < config.npeaks; i++)
    {
		std::size_t index = rand_long() % size;
        while (assigned[index])
        {
            index = rand_long() % size;
        }
        assigned[index] = true;
        assigned_cells++;
        map[index].continent = i;
        globals.continents[i].addCell(index);
		active[i].push_back(index);

		// add neighbors to active
        for (int j = 0; j < map[index].neighbors.size(); j++)
        {
			std::size_t neighborIdx = map[index].neighbors[j];
            if (!assigned[neighborIdx])
            {
                active[i].push_back(neighborIdx);
			}
        }
	}

	// Grow continents until all cells are assigned
    while (assigned_cells < size)
    {
		std::size_t continetIdx = rand_long() % config.npeaks;
        if (active[continetIdx].empty()) {
            continue;
        }
		// Get next cell from the continent's active queue
        std::size_t cellIdx = pop_random_i(active[continetIdx]);
        if (assigned[cellIdx]) {
            continue;
        }
        assigned[cellIdx] = true;
        assigned_cells++;
        map[cellIdx].continent = continetIdx;
		globals.continents[continetIdx].addCell(cellIdx);
        // add neighbors to active
        for (int j = 0; j < map[cellIdx].neighbors.size(); j++)
        {
            std::size_t neighborIdx = map[cellIdx].neighbors[j];
            if (!assigned[neighborIdx])
            {
                active[continetIdx].push_back(neighborIdx);
            }
        }
    }
	// clean up continents
    if (assigned_cells != size)
    {
        for (std::size_t i = 0; i < map.size(); i++)
        {
            if (map[i].continent == -1)
            {
                // Find nearest continent by checking neighbors
                for (std::size_t j = 0; j < map[i].neighbors.size(); j++)
                {
                    int neighborContinent = map[map[i].neighbors[j]].continent;
                    if (neighborContinent != -1)
                    {
                        map[i].continent = neighborContinent;
                        globals.continents[neighborContinent].addCell(i);
                        break;
                    }
                }

                // If still no continent, find closest by distance
                if (map[i].continent == -1)
                {
                    float minDist = std::numeric_limits<float>::max();
                    int closestContinent = 0;

                    for (int j = 0; j < config.npeaks; j++)
                    {
                        float dist = distance(voronoi_points[i], globals.continents[j].getCenter());
                        if (dist < minDist)
                        {
                            minDist = dist;
                            closestContinent = j;
                        }
                    }

                    map[i].continent = closestContinent;
                    globals.continents[closestContinent].addCell(i);
                }
            }
        }
    }
    // Create the plates
    for (int i = 0; i < config.npeaks; i++)
    {
        globals.continents[i].setDirection(sf::Vector2f(
            RandomBetween(-1.0f, 1.0f),
            RandomBetween(-1.0f, 1.0f)
		));
		globals.continents[i].setAge(RandomBetween(0.5f, 1.0f)); // Age in billions of years
		globals.continents[i].finishContinent(voronoi_points, map);

		globals.continents[i].setHeight(RandomBetween(0.3f, 0.7f)); // Initial height (is replaced later)
	}
}


void continent_interaction(std::vector<Cell>& map, 
    GlobalWorldObjects& globals,
    const std::vector<sf::Vector2f>& cellPositions,
    int mapGenMode = 3,
    int propagation_depth = 1
    )
{
    // Get the interactions between the continents
    /*
    1. Determine boundry cells
    2. Get direction similarity to neighbors and add height
    3. Propagate
    */

    for (std::size_t cellIndex = 0; cellIndex < map.size(); cellIndex++)
    {
        Cell& cell = map[cellIndex];
        int mainContinentId = cell.continent;

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
            continue;
        }
        // add to the boundry list of the main continent
        globals.continents[mainContinentId].addBoundryCell(cellIndex);

        Continent& mainContinent = globals.continents[mainContinentId];
        sf::Vector2f mainDirection = mainContinent.getDirection();

        float totalHeightIncrease = 0.0f;
        bool hasVolcanicActivity = false;
        BoundaryType dominantBoundaryType = BoundaryType::None;

        for (int neighborContinentId : neighboringContinents)
        {
            Continent& neighborContinent = globals.continents[neighborContinentId];
            sf::Vector2f neighborDirection = neighborContinent.getDirection();

            // Calculate boundary normal
            sf::Vector2f cellPos = cellPositions[cellIndex];
            sf::Vector2f mainCenter = mainContinent.getCenter();
            sf::Vector2f neighborCenter = neighborContinent.getCenter();

			// Boundary normal poins from main continent to neighbor continent
			sf::Vector2f boundaryNormal = neighborCenter - mainCenter;
			float normalMagnitude = std::sqrt(boundaryNormal.x * boundaryNormal.x + boundaryNormal.y * boundaryNormal.y);
			if (normalMagnitude < 0.001f) { 
                continue; 
			}
			boundaryNormal.x /= normalMagnitude;
			boundaryNormal.y /= normalMagnitude;

			// Calculate relative motion vectors
			sf::Vector2f relativeVelocity = mainDirection - neighborDirection;

			// Project relative velocity onto boundary normal
			// Positive = converging, Negative = diverging, Zero = transform
			float convergenceRate = relativeVelocity.x * boundaryNormal.x + relativeVelocity.y * boundaryNormal.y;
			float tangentialRate = std::abs(relativeVelocity.x * (-boundaryNormal.y) + relativeVelocity.y * boundaryNormal.x);

            // Determine boundary type
			BoundaryType boundaryType;

			// Transform if tangentialRate is significantly higher than convergenceRate
			if (tangentialRate > std::abs(convergenceRate) * 1.5f && tangentialRate > 0.2f) {
				boundaryType = BoundaryType::Transform;
			}
            else if (convergenceRate > 0.2f) { // Converging
                if (mainContinent.plateType == PlateType::Continental &&
                    neighborContinent.plateType == PlateType::Continental) {
                    boundaryType = BoundaryType::ContinentalCollision;
                }
                else {
                    boundaryType = BoundaryType::Subduction;
                }
            }
            else if (convergenceRate < -0.2f) { // Diverging
                if (mainContinent.plateType == PlateType::Oceanic &&
                    neighborContinent.plateType == PlateType::Oceanic) {
                    boundaryType = BoundaryType::OceanicRift;
                }
                else if (mainContinent.plateType == PlateType::Continental &&
                    neighborContinent.plateType == PlateType::Continental) {
                    boundaryType = BoundaryType::ContinentalRift;
                }
                else {
                    boundaryType = BoundaryType::OceanicRift;
                }
            }
            else {
                boundaryType = BoundaryType::None;
			}

            if (dominantBoundaryType == BoundaryType::None)
            {
                dominantBoundaryType = boundaryType;
            }
            // Calculate height modifier based on boundary type
            float collisionIntensity = std::min(std::abs(convergenceRate) - 0.2f, 1.0f);
            float heightModifier = calculateBoundaryHeightModifier(boundaryType, convergenceRate,
                mainContinent.getAge(), neighborContinent.getAge());

            heightModifier *= (0.25f * collisionIntensity);

            switch (boundaryType)
            {
            case BoundaryType::ContinentalCollision:
            {
                // Himmalaya style
                heightModifier += RandomBetween(0.05f, 0.15f) * collisionIntensity;
                // Higher chance of volcanic activity in younger continents
                if (mainContinent.getAge() < 0.7f && RandomBetween(0.0f, 1.0f) < 0.3f * collisionIntensity)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            case BoundaryType::Subduction:
            {
                // Subduction zones like Andes
                bool isMainOceanic = mainContinent.plateType == PlateType::Oceanic;
                if (!isMainOceanic)
                {
                    // Continent side gets volcanic mountains
                    heightModifier += RandomBetween(0.08f, 0.18f);

                    // Higher chance of volcanic activity
                    if (RandomBetween(0.0f, 1.0f) < 0.6f)
                    {
                        hasVolcanicActivity = true;
                    }
                }
                else
                {
                    // Oceanic side gets deep trenches
                    heightModifier -= RandomBetween(0.05f, 0.12f);
                }
                break;
            }
            case BoundaryType::OceanicRift:
            {
                // Mid-ocean ridges
                heightModifier += RandomBetween(0.02f, 0.08f);
                // moderate volcanic activity
                if (RandomBetween(0.0f, 1.0f) < 0.3f)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            case BoundaryType::ContinentalRift:
            {
                // continental rifts valley formation
                heightModifier -= RandomBetween(0.05f, 0.12f);

                // minor volcanic activity
                if (RandomBetween(0.0f, 1.0f) < 0.15f)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            case BoundaryType::Transform:
            {
                // Minor height changes
                heightModifier += RandomBetween(-0.02f, 0.02f);
                // slight chance of volcanic activity
                if (RandomBetween(0.0f, 1.0f) < 0.05f)
                {
                    hasVolcanicActivity = true;
                }
                break;
            }
            default:
                break;
            }
            totalHeightIncrease += heightModifier;
        }
        // Apply total height increase to the cell - no clamping
        cell.height += totalHeightIncrease;
        if (hasVolcanicActivity) {
            cell.volcanicActivity = true;
            cell.height += RandomBetween(0.08f, 0.2f); // Allow tall volcanic peaks
        }

        // Propagation of mountain building effects
        propagateTerrainFeatures(map, globals, cellIndex, totalHeightIncrease, hasVolcanicActivity,dominantBoundaryType, propagation_depth);
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

        // Keep noise centered around 0 for natural variation
        noiseValue = noiseValue * 0.5f; // Range: [-0.5, 0.5]

        // Apply noise - no clamping, let heights be natural
        map[i].height += noiseValue * noiseStrength;
    }

}

// MEthod should be enum or something
// 1 = k-peaks, 2 = k-peaks continet, 3 = Continental, 4 = Continental 2
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
		int numOceanContinents = static_cast<int>(config.npeaks * config.oceanic_plate_ratio);

        for (int i = 0; i < config.npeaks; i++)
        {
            std::size_t index = rand_long() % map.size();

            
			// Select if the peak should be oceanic or continental
            if (RandomBetween(0.0f, 1.0f) < 0.1f)
            {
                globals.continents.emplace_back(Continent(i));
                globals.continents[i].plateType = PlateType::Mixed;
				map[index].height = normalDistPDF(0.5, 0.2);
				numOceanContinents--;
            }
            if (numOceanContinents > 0)
            {
                globals.continents.emplace_back(Continent(i));
                globals.continents[i].plateType = PlateType::Oceanic;
                map[index].height = normalDistPDF(0.3, 0.1);
				numOceanContinents--;
            }
            else
            {
                globals.continents.emplace_back(Continent(i));
                globals.continents[i].plateType = PlateType::Continental;
                map[index].height = normalDistPDF(0.7, 0.2);
			}

            active.insert(std::end(active), std::begin(map[index].neighbors), std::end(map[index].neighbors));

            // Continents
            globals.continents[i].addCell(index);
            map[index].continent = i;
			globals.continents[i].setCenter(points[index]); // Set the center of the continent to the first cell added

			globals.continents[i].setAge(RandomBetween(0.5, 0.9));
			globals.continents[i].setDirection(sf::Vector2f(RandomBetween(-1.0, 1.0), RandomBetween(-1.0, 1.0)));
			globals.continents[i].setHeight(map[index].height);
            if (globals.continents[i].plateType == PlateType::Oceanic)
            {
                globals.continents[i].crustThickness = RandomBetween(
                    config.oceanic_crust_thickness * 0.8f,
					config.oceanic_crust_thickness * 1.2f
                    );
                globals.continents[i].baseDensity = RandomBetween(
					0.8f,
					1.2f
                    );
            }
            else
            {
                globals.continents[i].crustThickness = RandomBetween(
                    config.continental_crust_thickness * 0.8f,
                    config.continental_crust_thickness * 1.2f
                );
                globals.continents[i].baseDensity = RandomBetween(
                    0.8f,
                    1.2f
				);
            }

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

			Cell& cell = map[index];
            for (std::size_t j = 0; j < map[index].neighbors.size(); j++)
            {
				size_t neighborIdx = map[index].neighbors[j];
                // small probability of random height increase, THIS is heavily up to tuning for interesting maps
                // Also should be reconsidered
                if (RandomBetween(0.0, 1.0) < config.prob_of_island && height_sum < config.dist_from_mainland && count_values > 1)
                {
                    map[neighborIdx].height = normalDistPDF(0.7, 0.3);
                    active.insert(std::begin(active), std::begin(map[neighborIdx].neighbors), std::end(map[neighborIdx].neighbors));
					
					for (std::size_t k = 0; k < map[neighborIdx].neighbors.size(); k++)
					{
                        globals.continents[map[index].continent].addCell(map[neighborIdx].neighbors[k]);
                        map[map[neighborIdx].neighbors[k]].continent = map[index].continent;
                    }
                }
				// check if neighbor has height
                if (map[neighborIdx].height != 0.f)
                {
                    height_sum = height_sum + map[neighborIdx].height;
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
						map[neighborIdx].continent = map[index].continent;
                    }
                }
            }
            if (count_values == 0) { active.push_back(map[index].id); }
            else {
                map[index].height = (height_sum / count_values) + RandomBetween(-config.delta_max_neg, config.delta_max_pos);
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

		if (config.height_method == 2)
		{
            continent_interaction(map, globals, points, config.height_method);
        }
		cleanUpContinents(map, globals);
        
        // Simplex noise overlay
        
        rise(map); // calculate the rise of the map with the new height values

		simulateErosion(map, globals, config); // simulate erosion on the height values
        simplex_noise_continent(map, globals, config, points);
        rise(map); // calculate the rise of the map with the new height values

    }
	else if (config.height_method == 3)
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
        continent_interaction(map, globals, points);

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
    else if (config.height_method == 4)
    {
		// Method 4 is an improved continental generation method
		continent_generation(map, globals, config, voronoi_points);
		assignPlateTypes(globals.continents, config);
		setBaseElevations(map, globals);
		continent_interaction(map, globals, points);
		simulateErosion(map, globals, config);
		simplex_noise_continent(map, globals, config, points);
		rise(map); // calculate the rise of the map with the new height values
		smooth_height(map, config.rise_threshold, 1, config.smooth_method); // smooth the height values
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
    // Find actual equator
	float equatorY = MAXHEIGHT / 2;
    if (!globals.convergenceLines.empty())
    {
		int middleIndex = globals.convergenceLines.size() / 2;
		equatorY = globals.convergenceLines[middleIndex] * MAXHEIGHT;
    }
    // Planetary temperature modifiers
	float baseTemp = globals.globalTempAvg;
	float equatorToPoleGradient = globals.planetaryParams.equatorToPoleTemp;
	float greenhouseBoost = globals.planetaryParams.greenhouseEffectFactor;
	float axisTilt = globals.planetaryParams.axialTilt;

    float atmosphericRetention = std::sqrt(globals.planetaryParams.atmosphere.totalPressure);

    for (int i = 0; i < map.size(); i++)
    {
        float temp = 0.f;

        // 1. Latitute based on equator
        float distFromEquator = std::abs(points[i].y - equatorY);
        float maxDistFromEquator = MAXHEIGHT * 0.5f; // Maximum distance (pole)
        float normalizedLatitude = std::min(distFromEquator / maxDistFromEquator, 1.0f);

        // Temperature decrease from equator to pole on planetary gradient
        float latTemp = baseTemp - (equatorToPoleGradient * std::pow(normalizedLatitude, 1.3f));
        temp += latTemp;

        // 2. Altitude modifier
        // Temp decreases with altitude (standard lapse rate ~6.5°C per km, scaled by atmospheric retention)
        // ocean depth increases temp slightly
        if (map[i].height >= globals.seaLevel)
        {
			// Land: standard lapse rate scaled by atmospheric retention
			float lapseRate = 6.5f / atmosphericRetention; // °C per km
			float elevationKm = (map[i].height - globals.seaLevel) * 10.0f; // in km
            temp -= clamp(lapseRate * elevationKm,70.0f,-50.0f);
        }
        else
		{
			// Ocean: slight increase in temp with depth
			float lapseRate = 6.5f / (atmosphericRetention * 5.0f); // Reduced lapse rate for ocean depth
			float depth = (globals.seaLevel - map[i].height) * 5.0f; // in km
            temp -= lapseRate * depth;
        }

        // 3. Distance from ocean (continental effect)
		// land far away from ocean is more extreme
		// stronger on less atmosphere circulation
		float continentalityFactor = 1.0f - std::abs(normalizedLatitude - 0.5f) * 2.0f; // Strongest at mid-latitudes
        // Ocean cells use distToOcean for their distance to land, the continental effect is only for land
        float oceanProximityEffect = map[i].oceanBool ? 0.f : map[i].distToOcean * 0.3f * continentalityFactor;

		// Effect depends on latitude and atmospheric retention
		float latitudeFactor = 1.0f - std::abs(normalizedLatitude - 0.5f) * 2.0f; // Strongest at mid-latitudes
        temp += oceanProximityEffect * latitudeFactor;

        // 4. Axial tilt
		// higher tilt = more seasonal variation
		// for now we just add a small modifier based on latitude
		float tiltEffect = (axisTilt / 23.5f) * normalizedLatitude * 3.0f;
		temp += RandomBetween(-tiltEffect, tiltEffect);

        // 5. Atmospheric Composition
		// Greenhouse effect
        temp += greenhouseBoost * (1.0f - normalizedLatitude * 0.3f);

        // Preassure effects heat retention
        if (globals.planetaryParams.atmosphere.totalPressure < 0.5f)
        {
            // Thin atmosphere: extreme day/night temperature swings (add randomness
			temp += RandomBetween(-10.0f, 10.0f) * (0.5f - globals.planetaryParams.atmosphere.totalPressure);
        }

		map[i].temp = temp;
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
        #pragma omp parallel for schedule(static)
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
        
		// Precompute wind direction and strength for each cell to optimize performance
        struct WindData {
            float windDir;
			float windStr;
			int bestNeighbor;
			float bestAlignment;
        };
		std::vector<WindData> windCache(map.size());

        #pragma omp parallel for schedule(static)
        for (std::size_t idx = 0; idx < map.size(); idx++)
        {
            float windDir = radians(map[idx].windDir);
			float windVariance = std::sin(run * 2.0f) * 20.f; // +/- 20 degrees
			windDir += radians(windVariance);

			int bestNeighbor = -1;
			float bestAlignment = -1.0f;
            for (int neighborIdx : map[idx].neighbors)
            {
                if (neighborIdx < 0 || neighborIdx >= map.size()) continue;
                float dx = points[neighborIdx].x - points[idx].x;
                float dy = points[neighborIdx].y - points[idx].y;
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
			windCache[idx] = { windDir, map[idx].windStr, bestNeighbor, bestAlignment };
        }


        for (int step = 0; step < advectionSteps; step++)
        {
            std::vector<float> newMoisture(map.size(), 0.0f);
            
            for (std::size_t idx = 0; idx < cellOrder.size(); idx++)
            {
				std::size_t i = cellOrder[idx];

                if (moisture[i] < 0.01f) continue;

                float cellMoisture = moisture[i];
				const WindData& windData = windCache[i];

                // Calculate precipitation
                float precipitationAmount = 0.0f;

                // 1. Baseline natural condensation (happens everywhere with sufficient moisture)
                if (cellMoisture > 10.0f)  // Only if there's enough moisture
                {
                    float moistureFactor = std::min(cellMoisture / 100.0f, 1.0f);
                    float baseCondensation = condensation_rate * moistureFactor * 12.0f;

					// Vary ocndensation rate per run to simulate seasonal changes
					float seasonalFactor = 0.8f + 0.4f * (run / static_cast<float>(runs));

                    precipitationAmount += baseCondensation * seasonalFactor;
                }

                // 2. Temperature-driven condensation (colder air holds less moisture)
                if (map[i].temp < 20.0f && cellMoisture > 20.0f)
                {
                    float coldFactor = clamp((20.0f - map[i].temp) / 30.0f, 1.0f, 0.0f);  // Stronger at colder temps
                    precipitationAmount += cellMoisture * 0.08f * coldFactor;
                }

                // 3. Orographic precipitation (mountains force uplift)
                if (windData.bestNeighbor >= 0 && !map[i].oceanBool)  // Only over land
                {
                    float heightDiff = map[windData.bestNeighbor].height - map[i].height;
                    if (heightDiff > 0.015f)  // Lower threshold for gentler slopes
                    {
                        float liftFactor = std::tanh(orographic_factor * heightDiff);
                        precipitationAmount += cellMoisture * liftFactor * 0.35f;
                    }
                }

                // 4. Oceanic storm precipitation (based on moisture and wind)
                if (map[i].oceanBool && cellMoisture > 50.0f && windData.windStr > 0.6f)
                {
                    float stormFactor = (windData.windStr - 0.6f) * 2.5f;  // Stronger winds = more storms
                    precipitationAmount += cellMoisture * 0.06f * stormFactor;
                }

                // Cap precipitation to avoid depleting all moisture
                precipitationAmount = std::min(precipitationAmount, cellMoisture * 0.65f);

                // Add precipitation
                newPrecipitation[i] += precipitationAmount;

                // Remaining moisture after precipitation
                float remainingMoisture = cellMoisture - precipitationAmount;

                // Transfer moisture to downwind neighbor
                if (windData.bestNeighbor >= 0 && windData.bestAlignment > 0.0f)
                {
                    // Transfer efficiency based on wind and alignment
					// Vary transfer factor per run to simulate seasonal wind strength changes
                    float runVariation = 0.85 + 0.1f * std::cos(run * 1.5f);
                    float transferFactor = windData.windStr * std::pow(std::max(0.0f, windData.bestAlignment), 0.4f) * runVariation;
                    float transferredMoisture = remainingMoisture * transferFactor;

                    // Rain shadow effect (descending terrain)
                    if (!map[i].oceanBool)  // Only apply to land
                    {
                        float heightDiff = map[windData.bestNeighbor].height - map[i].height;
                        if (heightDiff < -0.02f)
                        {
                            transferredMoisture *= 0.70f;  // Rain shadow dries air
                        }
                    }

                    // Natural moisture loss during transport
                    transferredMoisture *= (1.0f - moisture_loss_rate);

                    newMoisture[windData.bestNeighbor] += transferredMoisture;
                    newMoisture[i] += remainingMoisture * (1.0f - transferFactor);
                }
                else
                {
                    // No good neighbor - moisture either precipitates or dissipates
                    newPrecipitation[i] += remainingMoisture * 0.25f;
                    newMoisture[i] += remainingMoisture * 0.75f;
                }
            }
            
			moisture = std::move(newMoisture);
        }

        // Step 3: Accumulate precipitation across runs
        #pragma omp parallel for schedule(static)
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
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
		float noise = simplexNoise.noise(points[i].x * 0.02f, points[i].y * 0.02f) * 2.0f; // small variation
        map[i].percepitation = std::max(0.0f, map[i].percepitation + noise); // ensure non-negative
	}


	// Step 5: Cap the precipitation values
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
        map[i].percepitation = std::min(map[i].percepitation, max_percipitation);
	}

    // Step 6: Smooth precipitation for gradual transitions
    std::vector<float> smoothedPrecip(map.size());
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
        float sum = map[i].percepitation * 2.5f;  // Weight center
        int count = 2.5f;

        for (int neighborIdx : map[i].neighbors)
        {
            sum += map[neighborIdx].percepitation;
            count += 1;
        }

        smoothedPrecip[i] = sum / static_cast<float>(count);
    }

    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < map.size(); i++)
    {
        map[i].percepitation = smoothedPrecip[i];
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
		float strengthModifier = 0.8f + 0.2f * (1.0f - distFromLine);

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
			terrainFactor = std::max(terrainFactor, 0.3f);
		}
		// Ocean vs land: ocean has stronger more consistent winds
		float surfaceFactor = map[i].oceanBool ? 1.05f : 0.95f;

		// Calculate final wind strength
		float finalStr = baseStr * strengthModifier * terrainFactor * surfaceFactor;
        finalStr = clamp(finalStr + RandomBetween(-0.1f, 0.1f), 1.f, 0.05f);

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

void calcClimateVariance(std::vector<Cell>& map, const std::vector<sf::Vector2f>& points, GlobalWorldObjects& globals, float maxHeight)
{
    // Calculate Climate variance for each cell
    // 1. Ocean vs land
    // 2. coastal proximity
	// 3. latitude
	// 4. altitude
	// 5. atmospheric pressure

	float equatorY = maxHeight / 2;
    if (!globals.convergenceLines.empty())
    {
		int middleIndex = globals.convergenceLines.size() / 2;
		equatorY = globals.convergenceLines[middleIndex] * maxHeight;
    }
	float pressureFactor = 1.0f / std::sqrt(globals.planetaryParams.atmosphere.totalPressure + 0.1f);

    for (std::size_t i = 0; i < map.size(); i++)
    {
        Cell& cell = map[i];

        float distFromEquator = std::abs(points[i].y - equatorY);
        float maxDist = maxHeight * 0.5f;
        float normalizedLatitude = std::min(distFromEquator / maxDist, 1.0f);

        // == Temperature variance ==
        float baseTempVar = 5.0f;

        // Ocean vs land
        if (cell.oceanBool)
        {
            baseTempVar *= 0.3f; // oceans have lower temp variance
        }
        else if (cell.coastBool)
        {
            baseTempVar *= 0.6f; // coastal areas have moderate temp variance
        }
        else
        {
            float continentalFactor = 1.0f + std::min(cell.distToOcean * 0.05f, 0.5f);
            baseTempVar *= continentalFactor;
        }
        // Latitude effect
        float latitudeVarFactor = 1.0f + normalizedLatitude * 0.8f;
        baseTempVar *= latitudeVarFactor;
        // Altitude effect
        if (cell.height > globals.seaLevel)
        {
            float altitudeFactor = 1.0f + (cell.height - globals.seaLevel) * 0.5f;
            baseTempVar *= altitudeFactor;
        }
        // Atmospheric pressure effect
        baseTempVar *= pressureFactor;
        cell.tempVariance = baseTempVar;

        // == Wind Variance ==
        float baseWindVar = 0.1f;
        // coastal effect
        if (cell.coastBool)
        {
            baseWindVar *= 1.5f;
        }
        // mid-latitudes tend to have more variable winds
        float midLatitudeFactor = 1.0f + (1.0f - std::abs(normalizedLatitude - 0.5f) * 2.0f) * 0.4f;
        baseWindVar *= midLatitudeFactor;

        cell.windStrVariance = baseWindVar;

        cell.windDirVariance = 15.0f + baseWindVar * 30.0f;

        // == Humidity Variance ==
        float baseHumidVar = 0.1f;
        // Dry areas have less humidity variance
        baseHumidVar *= (1.0f - cell.humidity) + 0.5f;
        // Coastal areas have less humidity variance
        if (cell.coastBool || cell.oceanBool)
        {
            baseHumidVar *= 0.7f;
        }
        cell.humidityVariance = baseHumidVar;

        // == Precipitation Variance ==
        // I set precip varinace to be a percentage of mean precip
        float basePrecipVar = 0.4f;
        // Tropical areas have higher precip variance
        if (normalizedLatitude < 0.3f)
        {
            basePrecipVar *= 1.5f;
        }
        // arid areas have higher precip variance
        if (cell.percepitation < 15.0f && cell.percepitation > 0.1f)
        {
            basePrecipVar *= 1.5f;
        }
        // Stable oceanic climate has lower precip variance
        if (cell.oceanBool)
        {
            basePrecipVar *= 0.6f;
        }
        cell.percepitationVariance = basePrecipVar;
    }
}

