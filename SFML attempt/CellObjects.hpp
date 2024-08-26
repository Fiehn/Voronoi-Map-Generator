#pragma once
#include <vector>
#include <string>
#include <SFML/System/Vector2.hpp>


class Biome {
public:
	std::string name = "Biome"; // Name of biome
	int id = 0; // ID of biome

	sf::Color color = sf::Color::White; // Color of biome

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

	std::vector<int> validNeighbors; // Biome ID's that can be next to this biome

	// Constructor
	Biome(std::string name, int id, float avgTemp, float avgRain, float avgHumidity, float avgElevation, float avgWindStr, std::vector<int> validNeighbors, sf::Color color) {
		this->name = name;
		this->id = id;
		this->avgTemp = avgTemp;
		this->avgRain = avgRain;
		this->avgHumidity = avgHumidity;
		this->avgElevation = avgElevation;
		this->avgWindStr = avgWindStr;
		this->validNeighbors = validNeighbors;
		this->color = color;
	}

	// Returns the probability of biome in cell (0-1)
	float probabilityOfBiome(float temp, float rain, float humidity, float elevation, float windStr);

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

private:
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



float Biome::probabilityOfBiome(float temp, float rain, float humidity, float elevation, float windStr) {
	float tempProb = 1 - abs(avgTemp - temp) / 100;
	float rainProb = 1 - abs(avgRain - rain) / 100;
	float humidityProb = 1 - abs(avgHumidity - humidity) / 100;
	float elevationProb = 1 - abs(avgElevation - elevation) / 100;
	float windStrProb = 1 - abs(avgWindStr - windStr) / 100;
	return (tempProb + rainProb + humidityProb + elevationProb + windStrProb) / 5;
}
