#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "flecs.h"
#include "HistoryData.hpp"

// ==================
// Stucts to keep data for language management
// ==================
enum class PhenomeType : uint8_t {
	Vowel,
	Consonant
};

// Phoneme features are made from different aspects of speech sounds
enum class LanVoicing : uint8_t {
	Voiced,
	Voiceless,
	NA // Not applicable (for vowels)
};
enum class LanPlace : uint8_t {
	Bilabial,
	Labiodental,
	Dental,
	Alveolar,
	Postalveolar,
	Retroflex,
	Palatal,
	Velar,
	Uvular,
	Pharyngeal,
	Glottal,
	NA // Not applicable for vowels
};
enum class LanManner : uint8_t {
	Plosive,
	Nasal,
	Trill,
	Tap,
	Fricative,
	LateralFricative,
	Approximant,
	LateralApproximant,
	NA // Not applicable for vowels
};
enum class LanHeight : uint8_t {
	Close,
	NearClose,
	CloseMid,
	Mid,
	OpenMid,
	NearOpen,
	Open,
	NA // Not applicable for consonants
};
enum class LanBackness : uint8_t {
	Front,
	Central,
	Back,
	NA // Not applicable for consonants
};
enum class LanRounding : uint8_t {
	Rounded,
	Unrounded,
	NA // Not applicable for consonants
};

struct Phoneme {
	char symbol[8]; // UTF-8 symbol
	PhenomeType type;

	// Consonant features
	LanVoicing voicing = LanVoicing::NA;
	LanPlace place = LanPlace::NA;
	LanManner manner = LanManner::NA;

	// Vowel features
	LanHeight height = LanHeight::NA;
	LanBackness backness = LanBackness::NA;
	LanRounding rounding = LanRounding::NA;

	bool IsVowel() const { return type == PhenomeType::Vowel; }
	bool IsConsonant() const { return type == PhenomeType::Consonant; }
	bool IsVoiced() const { return voicing == LanVoicing::Voiced; }
	bool IsVoiceless() const { return voicing == LanVoicing::Voiceless; }
};

struct ConceptDatabase {
	std::vector<uint32_t> concept_ids;      // Unique identifier for each concept
	std::vector<std::string> concept_names; // Human-readable names for concepts
	//std::vector<uint32_t> semantic_categories; // Bitmask of SemanticCategory for each concept 
};
struct IPADatabase {
	std::vector<Phoneme> phonemes;
	std::unordered_map<std::string, size_t> symbol_to_index;

	std::vector<size_t> vowel_indices;
	std::vector<size_t> consonant_indices;

	bool LoadFromCSV(const std::string& filepath);
	const Phoneme* GetPhonemeBySymbol(const std::string& symbol) const;
	const Phoneme* GetPhonemeByIndex(size_t index) const;
};

// =================
// LanguageManager - Manages creation and storage of languages
// =================
class LanguageManager
{
public:
	LanguageManager() = default;

	void initialize(flecs::world& world);

	// === Creation Functions ===
	void create_proto_language(flecs::world& world, Cell& cell);
	void derive_language(flecs::world& world, flecs::entity parent_language_entity, Cell& cell);



	// === Accessors From Database ===
	const IPADatabase& GetIPADatabase() const { return m_ipa_database; }
	const ConceptDatabase& GetConceptDatabase() const { return m_concept_database; }
	Phoneme GetRandomPhoneme(bool vowel_only = false) const;
	Phoneme GetPhonemeBySymbol(const std::string& symbol) const;
	Phoneme GetPhonemeByIndex(size_t index) const;
	std::string GetConceptNameByID(uint32_t concept_id) const;
	uint32_t GetConceptIDByName(const std::string& concept_name) const;

	// === Evolution Functions ===
	void evolve(flecs::world& world, History& history, int32_t tick);

private:
	IPADatabase m_ipa_database;
	ConceptDatabase m_concept_database;

	bool LoadIPADatabase(const std::string& filepath);
	bool LoadConceptDatabase(const std::string& filepath);


};