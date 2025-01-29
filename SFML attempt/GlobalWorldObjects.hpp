class GlobalWorldObjects; // Forward declaration

#pragma once
#include <vector>
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

	std::vector<std::size_t> snowCells; // Cells that are covered in snow
	std::vector<std::size_t> treeCells; // Cells that do not have trees because of altitute
	std::vector<std::size_t> riverCells; // Cells that are part of a river
	std::vector<std::size_t> lakeCells; // Penamn formula (Loook up)
	std::vector<std::size_t> coastCells; // Cells that are part of the coast
	std::vector<std::size_t> oceanCells; // Cells that are part of the ocean
	std::vector<Biome> biomes; // List of biomes in the world
	std::vector<float> convergenceLines; // Convergence lines for wind and ocean currents: Given in y coordinates from 0 to 1 (0 being the top of the map) (0.5 being the equator) (The buttom of the map should not be included)
	std::vector<float> windDirection; // Wind direction for each convergence line (0 to 360 degrees) (0 being north) (will be the direction of the wind in the zone below the convergence line)
	std::vector<float> windStrength; // Wind strength for each convergence line (0 to 1) (1 being the strongest) (will be the strength of the wind in the zone below the convergence line)

	std::vector<River> rivers; // List of rivers in the world
	std::vector<Lake> lakes; // List of lakes in the world

	GlobalWorldObjects();
	void clearGlobals();

	void generateBiomes();
	void addBiome(std::string name, sf::Color color);
	void setConvergenceLines(std::vector<float> lines, std::vector<float> directions, std::vector<float> strength);
	void generateConvergenceLines(int nrLines, float windstr_alpha, float windstr_beta);
	std::vector<float> getConvergenceLines() const;
	void setGlobalTemp(float temp);
	void setSeaLevel(float level);
	void setGlobalSnowline(float snowline);
	void setGlobalTreeline(float treeline);

};


