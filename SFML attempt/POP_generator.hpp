#pragma once
#include "POP_Manager.hpp"
#include "Voronoi.hpp"
#include "Religion.hpp"
#include "GlobalWorldObjects.hpp"
#include "mapconfig.hpp"

// ===== Generating the Initial World POPs (cultures, religions, languages, and population) =====
class PopGenerator
{
public:
	// Constructor with seed
	PopGenerator(unsigned int seed = 0);

	// Populate the world with initial POPs
	void GenerateInitialPopulation(PopManager& popManager,
		const MapConfig& config,
		const vor::Voronoi& map,
		const GlobalWorldObjects& globals);

	// Flood out from starting locations
	void FirstStepExpansion(PopManager& popManager,
		const MapConfig& config,
		const vor::Voronoi& map,
		const GlobalWorldObjects& globals);

private:
	// Starting Location Selection
	std::vector<size_t> selectStartingLocations(const vor::Voronoi& map, size_t num_locations);
	// First Cultures Generation



};




