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
	void addBiome(std::string name, float avgTemp, float avgRain, float avgHumidity, float avgElevation, float avgWindStr, bool isWater, sf::Color color);
	void setConvergenceLines(std::vector<float> lines, std::vector<float> directions, std::vector<float> strength) { convergenceLines = lines; windDirection = directions; windStrength = strength; }
	void generateConvergenceLines(int nrLines, float windstr_alpha, float windstr_beta);
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
	biomes.clear();
}

void GlobalWorldObjects::generateConvergenceLines(int nrLines, float windstr_alpha = 2, float windstr_beta = 2)
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
		strength.push_back(betaDist(windstr_alpha,windstr_beta));
	}
	setConvergenceLines(lines, directions, strength);
}

void GlobalWorldObjects::addBiome(std::string name, float avgTemp, float avgRain, float avgHumidity, float avgElevation, float avgWindStr, bool isWater, sf::Color color)
{
	int id = biomes.size();
	Biome biome(name, id, avgTemp, avgRain, avgHumidity, avgElevation, avgWindStr, isWater, color);
	biomes.push_back(biome);

}

void GlobalWorldObjects::generateBiomes()
{
	addBiome("Tundra", -10.f, 0.1f, 0.1f, 0.1f, 0.1f, false, sf::Color::White);
	addBiome("Forest", 0.f, 0.f, 0.f, 0.f, 0.f, false, sf::Color::Green);
	//std::vector<std::string> names = { "Tundra", "Taiga", "Temperate Forest", "Tropical Rainforest", "Desert", "Savanna", "Grassland", "Shrubland", "Boreal Forest", "Chaparral", "Alpine", "Wetland", "Mangrove", "Coral Reef", "Kelp Forest", "Open Ocean", "Deep Ocean", "Ice Shelf", "Iceberg" };
}


