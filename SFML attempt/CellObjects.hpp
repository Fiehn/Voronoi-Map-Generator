#pragma once
#include <vector>
#include <string>
#include <SFML/System/Vector2.hpp>


class Biome {
public:
	std::string name = "Biome"; // Name of biome
	int id = 0; // ID of biome
	unsigned int numCells = 0; // Number of cells in biome

	sf::Color color = sf::Color::White; // Color of biome

	bool isOcean = false; // If biome is ocean

	int vegetationType = 0; // Primary type of vegetation in biome
	int vegetationDensity = 0; // Density of vegetation in biome
	int vegetationHeight = 0; // Height of vegetation in biome
	int vegetationColor = 0; // Color of vegetation in biome
	float vegetationVariety = 0; // Variety of vegetation in biome

	int animalType = 0; // Primary type of animal in biome
	int animalDensity = 0; // Density of animals in biome
	int animalSize = 0; // Size of animals in biome
	int animalColor = 0; // Color of animals in biome
	float animalVariety = 0; // Variety of animals in biome

	// Constructor
	Biome(std::string name, int id, float avgTemp, float avgRain, float avgHumidity, float avgElevation, float avgWindStr, bool isWater, sf::Color color) {
		this->name = name;
		this->id = id;
		this->avgTemp = avgTemp;
		this->avgRain = avgRain;
		this->avgHumidity = avgHumidity;
		this->avgElevation = avgElevation;
		this->avgWindStr = avgWindStr;
		this->color = color;
		this->isOcean = isWater;
	}

	// Getters
	float getAvgTemp() { return avgTemp; }
	float getAvgRain() { return avgRain; }
	float getAvgHumidity() { return avgHumidity; }
	float getAvgElevation() { return avgElevation; }
	float getAvgWindStr() { return avgWindStr; }

	// Setters
	void setVegetation(int type, int density, int height, int color, float variety) {
		vegetationType = type;
		vegetationDensity = density;
		vegetationHeight = height;
		vegetationColor = color;
		vegetationVariety = variety;
	}
	void setAnimal(int type, int density, int size, int color, float variety) {
		animalType = type;
		animalDensity = density;
		animalSize = size;
		animalColor = color;
		animalVariety = variety;
	}

	void setValues(std::vector<float> values) {
		avgTemp = values[0];
		avgRain = values[1];
		avgHumidity = values[2];
		avgElevation = values[3];
		avgWindStr = values[4];
		if (values.size() > 5) {
			if (values[5] > 1) {
				isOcean = true;
			}
		}
	}
	void setId(int id) {
		this->id = id;
	}
	
	// Values that are used to determine the probability for biome in cell
	float avgTemp = 0;
	float avgRain = 0;
	float avgHumidity = 0;
	float avgElevation = 0;
	float avgWindStr = 0;
};

class River {
public:
	float len = 0;
	float maxWidth = 0;
	float maxDepth = 0;
	float flowSpeed = 0;
	std::vector<int> cells;
};
