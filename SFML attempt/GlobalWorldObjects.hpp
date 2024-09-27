#pragma once
#include "Voronoi.hpp"
#include "cell.hpp"
#include <SFML/Graphics.hpp>
#include "CellObjects.hpp"

class GlobalWorldObjects
{
public:
	float seaLevel = 0.5f; // The sea level of the world (0 to 1) (1 being the top of the map)
	float globalTempAvg = 15.f; // Average temperature of the world, more an abstract value than a real one
	float globalSnowline = 0.8f; // The line where snow starts to appear on the map (0 to 1) (1 being the top of the map)
	float globalTreeline = 0.8f; // The line where trees stop appearing on the map (0 to 1) (1 being the top of the map)
	float globalHumidity = 0.f; // Average humidity of the world, more an abstract value than a real one
	float globalPercepitation = 0.f; // Average percepitation of the world, more an abstract value than a real one

	std::vector<int> snowCells; // Cells that are covered in snow
	std::vector<int> treeCells; // Cells that do not have trees because of altitute
	std::vector<int> riverCells; // Cells that are part of a river
	std::vector<int> lakeCells; // Penamn formula (Loook up)
	std::vector<int> coastCells; // Cells that are part of the coast
	std::vector<int> oceanCells; // Cells that are part of the ocean
	std::vector<Biome> biomes; // List of biomes in the world
	std::vector<float> convergenceLines; // Convergence lines for wind and ocean currents: Given in y coordinates from 0 to 1 (0 being the top of the map) (0.5 being the equator) (The buttom of the map should not be included)
	std::vector<float> windDirection; // Wind direction for each convergence line (0 to 360 degrees) (0 being north) (will be the direction of the wind in the zone below the convergence line)
	std::vector<float> windStrength; // Wind strength for each convergence line (0 to 1) (1 being the strongest) (will be the strength of the wind in the zone below the convergence line)


	GlobalWorldObjects();
	void clearGlobals();

	void generateBiomes();
	void addBiome(std::string name, float avgTemp, float avgRain, float avgHumidity, float avgElevation, float avgWindStr, bool isWater, std::vector<bool> excludes, sf::Color color);
	void setConvergenceLines(std::vector<float> lines, std::vector<float> directions, std::vector<float> strength) { convergenceLines = lines; windDirection = directions; windStrength = strength; }
	void generateConvergenceLines(int nrLines);
	std::vector<float> getConvergenceLines() { return convergenceLines; }
	void setGlobalTemp(float temp);
	void setSeaLevel(float level);
	void setGlobalSnowline(float snowline);
	void setGlobalTreeline(float treeline);
};

void GlobalWorldObjects::setGlobalSnowline(float snowline)
{
	globalSnowline = clamp(snowline,1.f,0.f);
}
void GlobalWorldObjects::setGlobalTreeline(float treeline)
{
	globalTreeline = clamp(treeline,1.f,0.f);
}
void GlobalWorldObjects::setSeaLevel(float level)
{
	seaLevel = clamp(level,1.f,0.f);
	// There needs to be an update of all variables for cells
}
void GlobalWorldObjects::setGlobalTemp(float temp)
{
	globalTempAvg = temp;
}

GlobalWorldObjects::GlobalWorldObjects()
{
	seaLevel = RandomBetween(0.4f, 0.6f);
	globalTempAvg = RandomBetween(25.f,45.f);
	globalSnowline = 0.8f;
	globalTreeline = 0.8f;
	globalHumidity = 0.f;
	globalPercepitation = 0.f;
}
void GlobalWorldObjects::clearGlobals()
{
	snowCells.clear();
	treeCells.clear();
	riverCells.clear();
	lakeCells.clear();
	coastCells.clear();
	oceanCells.clear();
}

void GlobalWorldObjects::generateConvergenceLines(int nrLines)
{
	convergenceLines.clear();
	windDirection.clear();
	windStrength.clear();

	std::vector<float> lines;
	std::vector<float> directions;
	std::vector<float> strength;

	for (int i = 0; i < nrLines; i++)
	{
		// Pushback equally spaced lines
		lines.push_back((float)i / (float)nrLines);
		directions.push_back(RandomBetween(0.f, 360.f));
		strength.push_back(RandomBetween(0.2f, 0.9f));
	}
	setConvergenceLines(lines, directions, strength);
}

void GlobalWorldObjects::addBiome(std::string name, float avgTemp, float avgRain, float avgHumidity, float avgElevation, float avgWindStr, bool isWater, std::vector<bool> excludes, sf::Color color)
{
	int id = biomes.size();
	Biome biome(name, id, avgTemp, avgRain, avgHumidity, avgElevation, avgWindStr, isWater, excludes, color);
	biomes.push_back(biome);

}

void GlobalWorldObjects::generateBiomes()
{
	biomes.clear();
	// Biomes are added here
	//										0		1		2					3						4		5			6			7				8				9			10			11			12		13				14				15			16				17			18			
	//std::vector<std::string> names = { "Tundra", "Taiga", "Temperate Forest", "Tropical Rainforest", "Desert", "Savanna", "Grassland", "Shrubland", "Boreal Forest", "Chaparral", "Alpine", "Wetland", "Mangrove", "Coral Reef", "Kelp Forest", "Open Ocean", "Deep Ocean", "Ice Shelf", "Iceberg" };
	//std::vector<float> avgTemps = { -20.f, -10.f, 10.f, 25.f, 30.f, 25.f, 20.f, 15.f, 5.f, 20.f, -5.f, 10.f, 25.f, 25.f, 15.f, 10.f, 5.f, -10.f, -10.f };
	//std::vector<float> avgRains = { 0.1f, 0.3f, 0.6f, 0.9f, 0.1f, 0.3f, 0.4f, 0.5f, 0.3f, 0.4f, 0.2f, 0.7f, 0.8f, 0.9f, 0.8f, 0.7f, 0.6f, 0.1f, 0.1f };
	//std::vector<float> avgHumidities = { 0.1f, 0.3f, 0.6f, 0.9f, 0.1f, 0.3f, 0.4f, 0.5f, 0.3f, 0.4f, 0.2f, 0.7f, 0.8f, 0.9f, 0.8f, 0.7f, 0.6f, 0.1f, 0.1f };
	//std::vector<float> avgElevations = { 0.1f, 0.3f, 0.6f, 0.9f, 0.1f, 0.3f, 0.4f, 0.5f, 0.3f, 0.4f, 0.2f, 0.7f, 0.8f, 0.9f, 0.8f, 0.7f, 0.6f, 0.1f, 0.1f };
	//std::vector<float> avgWindStr = { 0.1f, 0.3f, 0.6f, 0.9f, 0.1f, 0.3f, 0.4f, 0.5f, 0.3f, 0.4f, 0.2f, 0.7f, 0.8f, 0.9f, 0.8f, 0.7f, 0.6f, 0.1f, 0.1f };
	//std::vector<std::vector<int>> valid = { {0,1,2,8,10,15,17,18}, {0,1,2,8,10,15}, {0,1,2,6,7,8,9,10,11,15}, {3,5,6,7,8,9,12,13,14,15}, {3,4,5,7,9}, {4,5,6,7,9}, {2,3,4,5,6,7}, {2,3,4,5,6,7,9,11,12,15}, {} }; // Add the ids of the biomes that are valid neighbors
	//											"Tundra"			"Taiga"			"Temperate Forest"			"Tropical Rainforest"		"Desert"	"Savanna"  "Grassland"			"Shrubland			
	////// NEEEEEEEEEEEEEEEEEEEDS TO BE FINISHED!!!!!!!!!!!

	std::vector<std::string> names = { "Taiga", "Temperate Forest", "Grasslands", "Desert", "Ocean", "Deep Ocean"};
	std::vector<float> avgTemps = { -10.f, 10.f, 20.f, 30.f, 10.f, 0.f };
	std::vector<float> avgRains = { 1.f, 10.f, 5.f, 0.01f, 1.f, 0.f };
	std::vector<float> avgHumidities = { 0.01f, 0.9f, 0.5f, 0.0001f, 1.f, 0.f };
	std::vector<float> avgElevations = { 0.7f, 0.5f, 0.6f, 0.5f, 0.1f, 0.f };
	std::vector<float> avgWindStr = { 0.9f, 0.2f, 0.8f, 0.6f, 1.f, 0.f };
	std::vector<bool> isWater = { false, false, false, false, true, true };
	std::vector<std::vector<bool>> exclusions = { {true, true, true, false, true, false}, {true, true, true, false, true, false}, {true, true, true, true, true, false}, {false, false, true, true, true, false}, {true, true, true, true, true, true}, {true, true, true, true, true, true} };

	std::vector<sf::Color> colors = { sf::Color(0, 0, 255), sf::Color(0, 255, 0), sf::Color(255, 255, 0), sf::Color(255, 0, 0), sf::Color(0, 255, 255), sf::Color(125, 125, 255) };


	for (int i = 0; i < names.size(); i++)
	{
		addBiome(names[i], avgTemps[i], avgRains[i], avgHumidities[i], avgElevations[i], avgWindStr[i], isWater[i], exclusions[i], colors[i]);
	}
}


