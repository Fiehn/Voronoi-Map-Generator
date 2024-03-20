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
	std::vector<int> lakeCells;
	std::vector<int> coastCells;
	std::vector<int> oceanCells;

	GlobalWorldObjects();

};

GlobalWorldObjects::GlobalWorldObjects()
{
}

