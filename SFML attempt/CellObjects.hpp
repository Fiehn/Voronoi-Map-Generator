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
	Biome(std::string name, int id, sf::Color color) {
		this->name = name;
		this->id = id;
		this->color = color;
	}

	// Getters
	std::map<std::string, float> getValues() {
			return values;
		}

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

	void setValues(const std::map<std::string, float>& value) {
		values = value;
		
		if(values.at("Ocean") > 0.001f) 
		{
			isOcean = true;
			values.at("Ocean") = 1;
		}
		else
		{
			isOcean = false;
			values.at("Ocean") = 0;
		}
	}
	void setId(int id) {
		this->id = id;
	}
	
	// Values of the biome (average)
	std::map<std::string, float> values = {
		{"Temperature", 0},
		{"Rainfall", 0},
		{"Elevation", 0},
		{"Wind Strength", 0},
		{"Ocean", 0}
	};
};

// TODO: Biomes need names: https://en.wikipedia.org/wiki/List_of_biomes

class River {
public:
	float len = 0;
	float maxWidth = 0;
	float maxDepth = 0;
	float flowSpeed = 0;
	std::vector<int> cells;
};
