#pragma once
#include "GlobalWorldObjects.hpp"
#include "util.hpp"


void GlobalWorldObjects::setGlobalSnowline(float snowline)
{
	globalSnowline = clamp(snowline, 1.f, 0.f);
}
void GlobalWorldObjects::setGlobalTreeline(float treeline)
{
	globalTreeline = clamp(treeline, 1.f, 0.f);
}
void GlobalWorldObjects::setSeaLevel(float level)
{
	seaLevel = clamp(level, 1.f, 0.f);
	// There needs to be an update of all variables for cells
}
void GlobalWorldObjects::setGlobalTemp(float temp)
{
	globalTempAvg = temp;
}

GlobalWorldObjects::GlobalWorldObjects()
{
	seaLevel = RandomBetween(0.4f, 0.6f);
	globalTempAvg = RandomBetween(25.f, 45.f);
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
	rivers.clear();
	continents.clear();
	lakes.clear();
	cultures.clear(); 
	convergenceLines.clear();
	windDirection.clear();
	windStrength.clear();
}

void GlobalWorldObjects::generatePlanet(bool earthlike)
{
	if (earthlike)
		planetaryParams.initializeEarthLike();
	else
		planetaryParams.initialize();
}

void GlobalWorldObjects::setConvergenceLines(std::vector<float> lines, std::vector<float> directions, std::vector<float> strength)
{
	convergenceLines = lines;
	windDirection = directions;
	windStrength = strength;
}

void GlobalWorldObjects::generateConvergenceLines()
{
	convergenceLines.clear();
	windDirection.clear();
	windStrength.clear();

	std::vector<float> lines = planetaryParams.calculateCellBoundaries();
	std::vector<float> directions = planetaryParams.calculateWindDirections(lines);
	std::vector<float> strength = planetaryParams.calculateWindStrengths(lines);
	
	setConvergenceLines(lines, directions, strength);
}

void GlobalWorldObjects::addBiome(std::string name, sf::Color color)
{
	int id = biomes.size();
	Biome biome(name, id, color);
	biomes.push_back(biome);

}

void GlobalWorldObjects::generateBiomes()
{
	addBiome("Tundra", sf::Color::White);
	addBiome("Forest", sf::Color::Green);
	//std::vector<std::string> names = { "Tundra", "Taiga", "Temperate Forest", "Tropical Rainforest", "Desert", "Savanna", "Grassland", "Shrubland", "Boreal Forest", "Chaparral", "Alpine", "Wetland", "Mangrove", "Coral Reef", "Kelp Forest", "Open Ocean", "Deep Ocean", "Ice Shelf", "Iceberg" };
}

std::vector<float> GlobalWorldObjects::getConvergenceLines() const
{
	return convergenceLines;
}

void GlobalWorldObjects::initializeCultures(std::vector<Cell>& map)
{
	for (int i = 0; i < 5; i++)
	{
		// Random cell to start
		std::size_t cellId = rand() % map.size(); // Random cell id

		cultures.emplace_back(Culture(i, "Culture" + std::to_string(i), randomColor()));
		cultures[i].cells.push_back(cellId); // Add the cell to the culture
		cultures[i].overExtension.push_back(0.f); // Add the overExtension cost to the culture
		cultures[i].populatedCells = 1; // Set the number of populated cells
		map[cellId].culture = i; // Set the culture of the cell
	}
}