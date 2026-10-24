#pragma once
#include <string>
#include <map>
#include <SFML/Graphics.hpp>
#include <vector>
#include <glm/glm.hpp>
#include "cell.hpp"
#include <iostream>
#include "util.hpp"

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

enum class RiverMouth {
    Ocean,
    Lake,     // Ends in a lake, mouthLake is the index in globals.waterBodies
    Closed,   // Ends where the water is lost: no ocean and no lake to take it
    River     // A tributary, ends on a cell of the river it joins
};

// One river from the hydrology step: the cells from its source down to its mouth, in order, without
// backtracking. A river is the main stem from a mouth upstream along the larger branch, the smaller branch
// is a tributary of its own, whose path ends on the confluence cell (which belongs to the parent)
class River {
public:
    int id = 0;
    explicit River(int id) : id(id) {}

    std::vector<std::size_t> pathCells;
    float lengthKm = 0.f;         // Along the cell centres, a main stem includes the step to the water it flows into
    RiverMouth mouthType = RiverMouth::Closed;
    int mouthLake = -1;
    float mouthDischarge = 0.f;   // m3/s at the last cell the river owns
    int parentRiver = -1;
    std::vector<int> tributaries;

    float getLength() const { return lengthKm; }
    const std::vector<std::size_t>& getPathCells() const { return pathCells; }
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

	// The plate on the sphere: it rotates about eulerAxis (through the planet centre) at angularSpeed,
	// so the surface at p moves with velocityAt(p). centerDir is the normalised mean of its cell centres
	glm::vec3 centerDir = glm::vec3(0.f, 0.f, 1.f);
	glm::vec3 eulerAxis = glm::vec3(0.f, 1.f, 0.f);
	float angularSpeed = 0.f;
	glm::vec3 velocityAt(const glm::vec3& p) const { return angularSpeed * glm::cross(eulerAxis, p); }
	void calcCenterOnSphere(const std::vector<glm::vec3>& points);
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
    void setAge(float age) { this->age = age; }

	void addCell(std::size_t cell) { cells.push_back(cell); };
	std::vector<std::size_t> getCells() { return cells; };

    double getHeight() { return height; }
    float getAge() { return age; }

    void addBoundryCell(std::size_t cellId);

private:
    std::vector<std::size_t> boundCells;
    double height = 0.5;
};


