#pragma once
#include "POP_Manager.hpp"
#include "SphereVoronoi.hpp"
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
		const vor::SphereVoronoi& map,
		const GlobalWorldObjects& globals);

	// Flood out from starting locations
	void FirstStepExpansion(PopManager& popManager,
		const MapConfig& config,
		const vor::SphereVoronoi& map,
		const GlobalWorldObjects& globals);

private:
	// Starting Location Selection
	std::vector<size_t> selectStartingLocations(const vor::SphereVoronoi& map, size_t num_locations);
	// Proto-language generation
	std::vector<LanguageHandle> generateProtoLanguages(GlobalWorldObjects& globals,
		const std::vector<size_t>& starting_cells,
		const vor::SphereVoronoi& map);

	// Proto-religion generation
	std::vector<ReligionHandle> generateProtoReligions(GlobalWorldObjects& globals,
		const std::vector<size_t>& starting_cells,
		const vor::SphereVoronoi& map);

	// Generate Founding cultures 
	std::vector<CultureHandle> generateFoundingCultures(GlobalWorldObjects& globals,
		const std::vector<size_t>& starting_cells,
		const std::vector<LanguageHandle>& proto_languages,
		const std::vector<ReligionHandle>& proto_religions,
		const vor::SphereVoronoi& map);

	void createInitialPops(PopManager& popManager,
		const std::vector<size_t>& starting_cells,
		const std::vector<CultureHandle>& founding_cultures,
		const std::vector<LanguageHandle>& languages,
		const std::vector<ReligionHandle>& religions);
};




