#pragma once
#include "POP_Manager.hpp"
#include "Culture.hpp"
#include "Religion.hpp"
#include "Language.hpp"
#include "GlobalWorldObjects.hpp"
#include "Voronoi.hpp"
#include "util.hpp"
#include <random>
#include <algorithm>

// ===================
// PopGenerationConfig - Configuration for initial pop generation
// ===================
struct PopGenerationConfig {
	// Scale parameters
	float population_density = 1.0f; // Multiplier for population density
	float urbanization_rate = 0.3f; // Percentage of population in urban areas
	float coastal_preference = 1.2f; // Multiplier for coastal population preference

	// Cultural diversity
	int min_cultures = 5;		// Minimum number of cultures
	int max_cultures = 20;		// Maximum number of cultures
	float culture_mixing_rate = 0.1f; // Rate of cultural mixing

	// Religious diversity
	int min_religions = 3;		// Minimum number of religions
	int max_religions = 15;		// Maximum number of religions
	float religious_conversion_rate = 0.05f; // Rate of religious conversion

	// Linguistic diversity
	int min_languages = 4;		// Minimum number of languages
	int max_languages = 25;		// Maximum number of languages
	float language_shift_rate = 0.07f; // Rate of language shift

	// Economic parameters
	float wealth_variance = 0.3f; // Variance in wealth distribution
	float literacy_base = 0.2f;   // Base literacy rate
	float literacy_variance = 0.25f; // Variance in literacy rates

	// Random seed for generation
	unsigned int random_seed = 42;
};

// ===================
// PopGenerator - Generates initial population based on configuration
// ===================

class PopGenerator {
public:
	PopGenerator(unsigned int seed);

	// ===== Culture Generation =====
	std::vector<Culture> GenerateCultures(const PopGenerationConfig& config, const vor::Voronoi& map);

	// ===== Religion Generation =====
	std::vector<Religion> GenerateReligions(const PopGenerationConfig& config, const vor::Voronoi& map);

	// ===== Language Generation =====
	std::vector<Language> GenerateLanguages(const PopGenerationConfig& config, const vor::Voronoi& map);

	// ===== Population Generation =====
	void GenerateInitialPopulation(PopManager& popManager,
		const PopGenerationConfig& config,
		const vor::Voronoi& map,
		const GlobalWorldObjects& globals);

private:
	std::mt19937 m_rng; // Random number generator

	// ====== Helper Functions =====

	// Get suitable population for a cell based on its properties
	int32_t GetCellPopulation(const Cell& cell, const PopGenerationConfig& config);

	// Determine culture for a cell
	uint16_t DetermineCellCulture(size_t cellIndex,
		const Cell& cell,
		const std::vector<Culture>& cultures,
		const vor::Voronoi& map);

	// Determine religion for a POP
	uint16_t DeterminePopReligion(size_t cellIndex,
		const Cell& cell,
		const std::vector<Religion>& religions,
		const vor::Voronoi& map,
		uint16_t culture_id);

	// Generate random cultureal traits
	void GenerateCulturalTraits(Culture& culture);
	// Generate random religious traits
	void GenerateReligiousTraits(Religion& religion);
	// Generate random language traits
	void GenerateLinguisticTraits(Language& language);

	// Create Names for Cultures, Religions, and Languages
	std::string GenerateName(const std::string& type);
};


