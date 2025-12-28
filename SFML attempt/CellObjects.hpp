#pragma once
#include <string>
#include <map>
#include <SFML/Graphics.hpp>
#include <vector>
#include "cell.hpp"
#include <iostream>

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

    void setValues(const std::map<std::string, float>& value);

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

class River {
public:
	int id = 0;
    // Constructor
    River(int id);

    // Getters
    float getLength() const { return len; };
    std::vector<sf::Vector2f> getPath() { return path; };

    // functions
    void purgeRiver(std::vector<Cell>& map);

	void addCell(std::size_t cell);

    sf::VertexArray drawRiver();

	void finishRiver(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& voronoi_points) {
		calcPath(map, voronoi_points);
	};

	void addTributary(int tributary) { tributaries.push_back(tributary); };
	void setParentRiver(int parent) { parentRiver = parent; };

private:
    float len = 0;
    std::vector<std::size_t> cells;
    std::vector<sf::Vector2f> path;
	std::vector<int> tributaries;
	int parentRiver = -1;
    std::size_t endCell = 0;
    void calcLen() { len = static_cast<float>(path.size()); };
    void calcPath(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& voronoi_points);
};

class Lake {
public:
	int id = 0;
    
	// Constructor
	Lake(int id) {
		this->id = id;
	};

	// Getters
	float getArea() const { return area; };
	float getMaxDepth() const { return maxDepth; };
	std::vector<std::size_t> getCells() { return cells; };
	std::vector<sf::Vector2f> getBounds() { return bounds; };

	// Functions
	void addCell(std::size_t cell) { cells.push_back(cell); };
	void addBounds(sf::Vector2f bound) { bounds.push_back(bound); };
	
	void finishLake(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& voronoi_points) {
		calcBounds(voronoi_points, map);
		calcArea();
		calcMaxDepth();
	};

	sf::VertexArray drawLake();

private:
    float area = 0;
    float maxDepth = 0;
    std::vector<std::size_t> cells;
    std::vector<sf::Vector2f> bounds;

    void calcArea();
    void calcMaxDepth();
    void calcBounds(const std::vector<sf::Vector2f>& voronoi_points, const std::vector<Cell>& map);
	
};

enum class PlateType {
    Oceanic,
    Continental,
    Mixed
};

enum class BoundaryType {
    None, 
	ContinentalCollision, // Both continental -> himmalaya
	Subduction, // Oceanic under continental -> andes
	OceanicRift, // Both oceanic -> mid atlantic ridge
	ContinentalRift, // Both continental pulling apart -> east african rift
	Transform // Sliding past each other -> san andreas
};

class Continent {
public:
    int id;
    Continent(int id);
    std::vector<std::size_t> cells;
	
	PlateType plateType = PlateType::Continental;
    float age = 3.0f;
	float crustThickness = 35.0f; // in km
	float baseDensity = 2.7f; // g/cm^3

    float getIsostaticHeight() {
		float isostaticFactor = (crustThickness / 35.0f) * (3.0f / baseDensity);
        if (plateType == PlateType::Oceanic) {
			return - 0.2f + (isostaticFactor - 1.0f) * 0.3f; // Oceanic crust is thinner and denser
		}
        else if (plateType == PlateType::Continental) {
            return 0.4f + (isostaticFactor - 1.0f) * 0.5f; // Continental crust is thicker and less dense
        }
        else { // Mixed
            return - 0.05f + (isostaticFactor - 1.0f) * 0.35f;
		}
    }

	void setHeight(double height) { this->height = height; };
    void setDirection(sf::Vector2f direction) { this->direction = direction; };
    void setAge(float age) { this->age = age; }
	void setCenter(sf::Vector2f center) { this->center = center; };

	void addCell(std::size_t cell) { cells.push_back(cell); };
	std::vector<std::size_t> getCells() { return cells; };

    sf::Vector2f getDirection() { return direction; };
	sf::Vector2f getCenter() { return center; };
    double getHeight() { return height; }
    float getAge() { return age; }

    void addBoundryCell(std::size_t cellId);

    void generateBoundryLine(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& voronoi_points);

    sf::VertexArray drawDirectionArrows();

	sf::VertexArray drawBoundryLine(); 

    void finishContinent(const std::vector<sf::Vector2f>& voronoi_points, const std::vector<Cell>& map);

private:
    std::vector<std::size_t> boundCells;
    std::vector<sf::Vector2f> boundLines; 
	sf::Vector2f center;
    sf::Vector2f direction;
    double height = 0.5;

	void calcCenter(const std::vector<sf::Vector2f>& voronoi_points, const std::vector<Cell>& map);

};


