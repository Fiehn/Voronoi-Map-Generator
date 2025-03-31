class Cell; // Forward declaration

#pragma once
#include <vector>
#include <SFML/System/Vector2.hpp>


class Cell 
{  
public:
    unsigned int id; // Unique Id coming from the points vector
    Cell(int i) : id(i) { vertex.reserve(10); neighbors.reserve(10); }; // Constructor, am I doing this right?
    std::vector<int> vertex; // Id's of vertecies that corespond to the cell and are stored in voronoi_points this should be pointers?
    std::vector<int> neighbors; // Id's of the neighbors
    unsigned int vertex_offset = 0U; // Offset for the vertex buffer

    float height = 0.f; // Height of the cell, 1 = 8km above sealevel 
    float rise = 0.f; // Difference in height between the highest and the lowest neighbor cell (0 to 1)
    float temp = 0.f; // Temperature of the cell (Celsius)
    float windDir = 0.f; // Wind direction (0 to 360 degrees)
    float windStr = 0.f; // Wind strength (0 to 1)
    float humidity = 1.f; // Humidity of the cell (0 to 1)
    float percepitation = 0.f; // Percepitation of the cell ( > 0 )

	int continent = 0; // Continent of the cell
	bool volcanicActivity = false; // Has volcanic activity
	
    int biome = 0; // Biome of the cell
    std::vector<float> biome_prob; // Probabilities of each biome in the cell

	int distToOcean = std::numeric_limits<int>::max(); // Distance to the nearest ocean cell (how many cells to the ocean (shortest distance))

    bool oceanBool = false; // Is an ocean
    bool coastBool = false; // Is next to ocean

    bool riverBool = false; // Has a river
    float riverStr = 0.f; // River strength
	int riverId = -1; // River id

    bool lakeBool = false; // Has a lake
	int lakeId = -1; // Lake id

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

