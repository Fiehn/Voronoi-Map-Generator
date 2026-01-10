#pragma once
#include <string>
#include <vector>
#include <cstdint>

class Language {
public:
	uint16_t id;
	std::string name;
	
	// Linguistic characteristics
	float phonetic_complexity = 0.5f;		// 0: simple, 1: complex
	float prestige = 0.5f;					// 0: low prestige, 1: high prestige
	float attractiveness = 0.5f;			// 0: unattractive, 1: attractive
	float grammatical_complexity = 0.5f;	// 0: simple, 1: complex
	float lexical_richness = 0.5f;			// 0: limited vocabulary, 1: rich vocabulary
	
	// Demographics
	uint64_t total_speakers = 0; // Total number of speakers
	// uint64_t native_speakers = 0; // Number of native speakers (not implemented yet in the way the POPs are set up they can only speak one language)

	// Relationships
	uint16_t parent_language_id = 0; // Parent language (id of the language), 0 = proto-language
	std::vector<uint16_t> dialects; // Dialects that diverged from this language (id of the language)
	std::vector<uint16_t> derived_languages; // Languages that derived from this language (id of the language)

	// Writing system
	bool has_writing_system = false; // Whether the language has a writing system
	std::string script_name = "None"; // Name of the writing system/script

	// Constructor
	Language(uint16_t id, const std::string& name)
		: id(id), name(name) {}

	// Default constructor
	Language() : id(0), name("Proto-Language") {}

	float CalculateMutualIntelligibility(const Language& other) const {
		if (id == other.id) {
			return 1.0f; // Same language
		}
		float intelligibility = 1.0f;
		// Different parent languages reduce intelligibility
		if (parent_language_id != other.parent_language_id) {
			intelligibility -= 0.4f;
		}
		// Greater differences in linguistic characteristics reduce intelligibility
		intelligibility -= std::abs(phonetic_complexity - other.phonetic_complexity) * 0.1f;
		intelligibility -= std::abs(grammatical_complexity - other.grammatical_complexity) * 0.1f;
		intelligibility -= std::abs(lexical_richness - other.lexical_richness) * 0.1f;
		return std::max(0.0f, intelligibility);
	}
};