#pragma once
#include <vector>
#include <map>
#include <string>

// Forward declarations
class Cell;
class GlobalWorldObjects;
class Biome;

enum class ResourceType {
	// Metals
	Copper,
	Iron,
	Tin,
	Gold,
	Silver,
	Lead,

	// Materials
	Lumber,
	Stone,
	Clay,
	Coal,

	// Agriculture
	Grain,
	Fruit,
	Vegetables,
	Cotton,

	// Livestock
	Livestock,
	Sheep,
	Furs,
	Fish,
	Whales,

	// Luxury
	Spices,
	Gems,
	Dyes
};

std::string resourceTypeToString(ResourceType type);

class ExtractiveResource {
public:
	ExtractiveResource() = default;

	// Set amount of resource
	void setResourceAmount(ResourceType type, float amount);
	
	// Get amount of resource
	float getResourceAmount(ResourceType type);
	// Check if resource exists
	bool hasResource(ResourceType type);
	// Get all resources
	std::map<ResourceType, float> getAllResources() const;

	void clear() {
		resources.clear();
	}

private:
	std::map<ResourceType, float> resources;
};

namespace ResourceGen {
	// Generate resources for a cell based on its properties
	void generateCellResources(Cell& cell, const GlobalWorldObjects& globals);

	// Generate resources for the entire map
	void generateMapResources(std::vector<Cell>& map, const GlobalWorldObjects& globals);
	void applyResourceSmoothing(std::vector<Cell>& map, int smoothingPasses = 1);
	float getNeighborAverage(std::vector<Cell>& map, std::size_t cellIndex, ResourceType resType);

	// Helper functions
	float calculateFishAbundance(const Cell& cell);
	float calculateWhaleAbundance(const Cell& cell);
	float calculateMetalAbundance(const Cell& cell, ResourceType type);
	float calculateAgricultureAbundance(const Cell& cell, ResourceType type);
	float calculateLivestockAbundance(const Cell& cell, ResourceType type);
	float calculateLuxuryAbundance(const Cell& cell, ResourceType type);
	float calculateLumberAbundance(const Cell& cell);
	float calculateMaterialAbundance(const Cell& cell, ResourceType type);
}