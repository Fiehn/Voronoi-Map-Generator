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
}

void GlobalWorldObjects::setConvergenceLines(std::vector<float> lines, std::vector<float> directions, std::vector<float> strength)
{
	convergenceLines = lines;
	windDirection = directions;
	windStrength = strength;
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
		strength.push_back(betaDist(windstr_alpha, windstr_beta));
	}
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
