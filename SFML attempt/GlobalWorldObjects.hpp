#pragma once
#include "Voronoi.hpp"
#include "cell.hpp"
#include <SFML/Graphics.hpp>

class GlobalWorldObjects
{
public:
	float seaLevel = 0.5f;
	float globalTempAvg = 0.f;
	float globalSnowline = 0.8f;
	float globalTreeline = 0.8f;
	float globalHumidity = 0.f;
	float globalPercepitation = 0.f;

	std::vector<int> snowCells;
	std::vector<int> treeCells;
	std::vector<int> riverCells;
	std::vector<int> lakeCells; // Penamn formula (Loook up)
	std::vector<int> coastCells;
	std::vector<int> oceanCells;
	std::vector<float> convergenceLines; // Convergence lines for wind and ocean currents: Given in y coordinates from 0 to 1 (0 being the top of the map) (0.5 being the equator) (The buttom of the map should not be included)
	std::vector<float> windDirection; // Wind direction for each convergence line (0 to 360 degrees) (0 being north) (will be the direction of the wind in the zone below the convergence line)
	std::vector<float> windStrength; // Wind strength for each convergence line (0 to 1) (1 being the strongest) (will be the strength of the wind in the zone below the convergence line)

	GlobalWorldObjects();
	void clearGlobals();

	void setConvergenceLines(std::vector<float> lines, std::vector<float> directions, std::vector<float> strength) { convergenceLines = lines; windDirection = directions; windStrength = strength; }
	void generateConvergenceLines(int nrLines);
	std::vector<float> getConvergenceLines() { return convergenceLines; }

};

GlobalWorldObjects::GlobalWorldObjects()
{
	seaLevel = 0.5f;
	globalTempAvg = 0.f;
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


