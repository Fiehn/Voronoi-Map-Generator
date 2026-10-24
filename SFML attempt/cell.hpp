class Cell; // Forward declaration

#pragma once
#include <vector>
#include <limits>
#include "extractiveResources.hpp"


class Cell 
{  
public:
    unsigned int id; // Unique Id coming from the points vector
    Cell(int i) : id(i) { vertex.reserve(10); neighbors.reserve(10); };
    std::vector<int> vertex; // Id's of vertecies that corespond to the cell and are stored in voronoi_points this should be pointers?
    std::vector<int> neighbors; // Id's of the neighbors

    float height = 0.f; // Height of the cell (uncapped, sealevel is typically around 0.5)
    float rise = 0.f; // Difference in height between the highest and the lowest neighbor cell
    float temp = 0.f; // Temperature of the cell (Celsius)
    float windDir = 0.f; // Wind direction (0 to 360 degrees)
    float windStr = 0.f; // Wind strength (0 to 1)
    float humidity = 1.f; // Humidity of the cell (0 to 1)
    float percepitation = 0.f; // Percepitation of the cell ( > 0 )

	float tempVariance = 5.0f; // Temperature variance for the cell
	float windStrVariance = 0.2f; // Wind strength variance for the cell
	float windDirVariance = 15.f; // Wind direction variance for the cell
	float humidityVariance = 0.1f; // Humidity variance for the cell
	float percepitationVariance = 0.2f; // Percepitation variance for the cell

	int continent = -1; // Continent of the cell
	bool volcanicActivity = false; // Has volcanic activity
	
    int biome = 0; // Biome of the cell
    std::vector<float> biome_prob; // Probabilities of each biome in the cell

	// Cells to the nearest cell on the other side of the coast. Does double work: for land it is the distance to the ocean,
	// for ocean cells it is the distance to land
	int distToOcean = std::numeric_limits<int>::max();

    bool oceanBool = false; // Is an ocean
    bool coastBool = false; // Is next to ocean

    // Water balance from the hydrology step. Depths are in precipitation units (10 mm per year)
    float pet = 0.f; // Potential evapotranspiration, from temperature only
    float aet = 0.f; // Actual evapotranspiration of land (Budyko)
    float runoff = 0.f; // Precipitation that is not evapotranspired, land only
    float discharge = 0.f; // Water passing the cell in m3/s, the cell's own runoff plus everything upstream
    int downstream = -1; // The neighbour the water leaves to, -1 at the ocean and in closed lakes
    int waterBody = -1; // Index in globals.waterBodies when the cell is under a lake

    bool riverBool = false; // Has a river
    float riverStr = 0.f; // River strength
	int riverId = -1; // River id

    bool lakeBool = false; // Has a lake
	int lakeId = -1; // Lake id

    bool snowBool = false; // Has snow
    bool treeBool = true; // Has trees
    
    bool iceBool = false; // Is Ice cap

    int culture = -1; // Will be tied to POP calculations later

	ExtractiveResource resources; // Extractive resources in the cell (std::map<ResourceType, float>)

    ~Cell() {
        vertex.clear();
        neighbors.clear();
    }
};

