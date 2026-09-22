#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <SFML/Graphics/Color.hpp>

// ==================
// Culture Handle - Safe index-based references to a culture
// ==================
struct CultureHandle {
	uint16_t index = 0;			// Index in the cultures array
	uint16_t generation = 0;	// Generation for validity checking

	bool isValid() const { return generation != 0; };
	static CultureHandle Invalid() { return CultureHandle{ 0, 0 }; };

	bool operator==(const CultureHandle& other) const {
		return index == other.index && generation == other.generation;
	}
};

// ==================
// CultureData - Structure of Arrays for culture attributes
// ==================

struct CultureData
{
	// === Identity ===
	std::vector<std::string> names;			// Name of the culture
	std::vector<sf::Color> colors;			// Color representing the culture
	std::vector<uint16_t> primary_language_id; // Primary language of the culture
	std::vector<uint16_t> religion_id;		// Primary religion of the culture

	// === Heritage (Phylogenetic relationships) ===
	std::vector<uint16_t> parent_culture_id; // Parent culture (id of the culture), 0 = proto-culture
	std::vector<uint32_t> founding_dates;    // Date of founding (in simulation ticks)
	std::vector<size_t> origin_cell_ids;   // Cell where the culture originated
	std::vector<float> divergence_rates;    // Rate of divergence from parent culture (0-1)

	// === Cultural Traits ===
	std::vector<float> inovation_rate;        // Tech adoption rate
	std::vector<float> military_tradition;   // Military focus
	std::vector<float> individualism;        // Individualism vs collectivism
	std::vector<float> materialism;         // Materialism vs spiritualism
	std::vector<float> patriarchy;          // Patriarchal vs egalitarian
	std::vector<float> temperance;          // Temperance vs indulgence
	std::vector<float> closenessToNature;   // Harmony with nature
	std::vector<float> religiosity;         // Religiousness
	std::vector<float> artFocus;            // Focus on arts and culture
	std::vector<float> socialStratification; // Social hierarchy
	std::vector<float> expansionism;        // Expansionist tendencies
	std::vector<float> tradeFocus;         // Trade and commerce focus
	std::vector<float> multiculturalism;   // Openness to other cultures

	// === Technology State ===
	// BitMask for GRAND technologies discovered (64 techs max for now)
	std::vector<uint64_t> technology_bits;

	// === Geographic Adaptation ===
	// Influences expansions into different biomes
	std::vector<uint8_t> biome_adaptations; // Bitmask for biome adaptations
	std::vector<float> naval_capability;   // Naval capability level
	std::vector<float> mountain_adaptability; // Mountain adaptability level

	// === Historical Memory ===
	std::vector<std::vector<uint32_t>> remembered_events; // List of significant historical events (by event ID)

	// === Drift Accumulation ===
	// Track how much the culture has drifted from origin
	std::vector<float> cultural_drift_accum; // Accumulated cultural drift
	std::vector<float> language_drift_accum; // Accumulated language drift
	std::vector<float> religion_syncretism;  // Accumulated religion drift

	// === Derived Cultures ===
	std::vector<std::vector<uint16_t>> derived_culture_ids; // List of cultures that derived from this culture (ids of the cultures)

	// === Metadata ===
	size_t size() const {
		return names.size();
	}
	void reserve(size_t n);
	void clear();
	void resize(size_t n);
};

// ==================
// Culture - High-level culture class
// ==================




