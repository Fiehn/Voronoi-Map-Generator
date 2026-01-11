#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <array>

// =================
// Phenome System - IPA sound inventory
// =================
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

// =================
// PhonoTactics - Syllable structure rules
// =================
struct SyllableTemplate
{
	uint8_t onset_min = 0; // Minimum number of consonants before vowel
	uint8_t onset_max = 1; // Maximum number of consonants before vowel
	uint8_t nucleus_min = 1; // Minimum number of vowels
	uint8_t nucleus_max = 1; // Maximum number of vowels
	uint8_t coda_min = 0; // Minimum number of consonants after vowel
	uint8_t coda_max = 1; // Maximum number of consonants after vowel

	// Bitmask for allowed phonemes in each position
	uint64_t allowed_onset_mask = 0xFFFFFFFFFFFFFFFF;
	uint64_t allowed_nucleus_mask = 0xFFFFFFFFFFFFFFFF;
	uint64_t allowed_coda_mask = 0xFFFFFFFFFFFFFFFF;
};


// =================
// SoundChangeRule - Represents a phonological sound change
// =================
enum class EnvironmentPosition : uint8_t {
	WordInitial,
	WordFinal,
	BeforeVowel,
	AfterVowel,
	BetweenVowels,
	BeforeConsonant,
	AfterConsonant,
	BetweenConsonants,
	Anywhere
};
struct SoundChangeRule {
	uint8_t target_phoneme_index;		// Index in the phoneme inventory
	uint8_t replacement_phoneme_index;	// What it becomes
	EnvironmentPosition environment;	// Context for the change (Where it occurs)
	uint8_t context_before_idx = 0xFF; // Optional phoneme before target (0xFF = any)
	uint8_t context_after_idx = 0xFF;  // Optional phoneme after target (0xFF = any)
	float probability = 1.0f;         // Probability of change occurring (0.0 - 1.0)
	uint32_t application_date = 0;     // Simulation date when change starts applying
};

// =================
// Lexicon Entry - Individual word in the lexicon
// =================
struct LexiconEntry {
	uint32_t word_id;			// Unique identifier for the Concept
	std::string phonetic_form;  // Phonetic representation of the word
	uint32_t semantic_category; // Category bitmask (KINSHIP, NATURE, etc.)
	uint16_t frequency_rank = 1000;// Frequency rank (lower = more common)
	// Etymology
	uint16_t parent_language_id = 0; // Language from which this word is derived
	uint32_t parent_word_id = 0;    // Word ID in the parent language
	bool is_borrowed = false;    // Whether this word is a loanword

	// Compound words
	bool is_compound = false;         // Whether this word is a compound
	uint32_t compound_parts[2] = { 0, 0 }; // Word IDs of the parts if compound (max 2 parts for now)
};

// =================
// Semantic Categories - Predefined categories for lexicon entries
// =================
enum class SemanticCategory : uint32_t
{
	SC_KINSHIP = 1 << 0, // Mother, Father, Brother, Sister, etc.
	SC_BODY = 1 << 1,   // Head, Hand, Foot, Eye, etc.
	SC_NATURE = 1 << 2, // Tree, River, Mountain, Sun, etc.
	SC_ANIMALS = 1 << 3, // Dog, Cat, Bird, Fish, etc.
	SC_FOOD = 1 << 4,   // Bread, Meat, Fruit, Vegetable, etc.
	SC_ACTIONS = 1 << 5, // Run, Eat, Sleep, Speak, etc.
	SC_TOOLS = 1 << 6,  // Hammer, Knife, Wheel, etc.
	SC_QUALITIES = 1 << 7, // Big, Small, Hot, Cold, etc.
	SC_NUMBERS = 1 << 8, // One, Two, Three, etc.
	SC_COLORS = 1 << 9, // Red, Blue, Green, etc.
	SC_DIRECTIONS = 1 << 10, // North, South, East, West
	SC_TIME = 1 << 11, // Day, Night, Year, etc.
	SC_EMOTIONS = 1 << 12, // Happy, Sad, Angry, etc.
	SC_ABSTRACTS = 1 << 13, // Love, Freedom, Justice, etc.
	SC_POLITICAL = 1 << 14, // King, War, Peace, etc.
	SC_ECONOMY = 1 << 15  // Trade, Money, Market, etc.
};

// =================
// LanguageData - Structure of Arrays for language attributes
// =================
struct LanguageData
{
	// === Identity ===
	std::vector<std::string> names;			// Name of the language
	std::vector<uint16_t> parent_language_id; // Parent language (id of the language), 0 = proto-language
	std::vector<std::vector<uint16_t>> derived_languages; // List of derived language IDs

	// === Phylogenetic Relationships ===
	std::vector<uint32_t> divergence_dates;    // Date of divergence (in simulation ticks)
	std::vector<float> divergence_scores;    // Divergence score from parent language
	std::vector<size_t> origin_cell_ids;   // Cell where the language originated

	// === Phonology (stored in compact tables) ===
	std::vector<uint32_t> phoneme_inventory_offsets; // Start index in shared phoneme pool
	std::vector<uint32_t> phoneme_inventory_counts;  // Number of phonemes in inventory
	std::vector<SyllableTemplate> syllable_templates;

	// === Lexicon (stored seperately) ===
	std::vector<uint32_t> lexicon_offsets; // Start index in shared lexicon pool
	std::vector<uint32_t> lexicon_counts;  // Number of words in lexicon

	// === Sound Change History ===
	std::vector<uint32_t> sound_change_offsets; // Start index in shared sound change pool
	std::vector<uint32_t> sound_change_counts;  // Number of sound changes applied

	// === Linguistic Traits === 
	std::vector<float> phonetic_complexity; // Complexity of phoneme inventory (0-1)
	std::vector<float> grammatical_complexity; // Complexity of grammar (0-1)
	std::vector<float> lexical_richness; // Size and diversity of lexicon (0-1)
	std::vector<float> prestige; // Social prestige of the language (0-1)
	std::vector<float> attractiveness; // Attractiveness to speakers of other languages (0-1)

	// === active speakers tracking ===
	std::vector<uint32_t> active_speakers_counts; // Number of active speakers of the language

	// === Metadata ===
	std::vector<uint32_t> generations; // Generation for validity checking
	std::vector<bool> is_extinct; // Whether the language is extinct

	size_t size() const {
		return names.size();
	}
	void reserve(size_t n) {
		names.reserve(n);
		parent_language_id.reserve(n);
		derived_languages.reserve(n);
		divergence_dates.reserve(n);
		divergence_scores.reserve(n);
		origin_cell_ids.reserve(n);
		phoneme_inventory_offsets.reserve(n);
		phoneme_inventory_counts.reserve(n);
		syllable_templates.reserve(n);
		lexicon_offsets.reserve(n);
		lexicon_counts.reserve(n);
		sound_change_offsets.reserve(n);
		sound_change_counts.reserve(n);
		phonetic_complexity.reserve(n);
		grammatical_complexity.reserve(n);
		lexical_richness.reserve(n);
		prestige.reserve(n);
		attractiveness.reserve(n);
		active_speakers_counts.reserve(n);
		generations.reserve(n);
		is_extinct.reserve(n);
	}
	void resize(size_t n) {
		names.resize(n);
		parent_language_id.resize(n);
		derived_languages.resize(n);
		divergence_dates.resize(n);
		divergence_scores.resize(n);
		origin_cell_ids.resize(n);
		phoneme_inventory_offsets.resize(n);
		phoneme_inventory_counts.resize(n);
		syllable_templates.resize(n);
		lexicon_offsets.resize(n);
		lexicon_counts.resize(n);
		sound_change_offsets.resize(n);
		sound_change_counts.resize(n);
		phonetic_complexity.resize(n);
		grammatical_complexity.resize(n);
		lexical_richness.resize(n);
		prestige.resize(n);
		attractiveness.resize(n);
		active_speakers_counts.resize(n);
		generations.resize(n);
		is_extinct.resize(n);
	}
	void clear() {
		names.clear();
		parent_language_id.clear();
		derived_languages.clear();
		divergence_dates.clear();
		divergence_scores.clear();
		origin_cell_ids.clear();
		phoneme_inventory_offsets.clear();
		phoneme_inventory_counts.clear();
		syllable_templates.clear();
		lexicon_offsets.clear();
		lexicon_counts.clear();
		sound_change_offsets.clear();
		sound_change_counts.clear();
		phonetic_complexity.clear();
		grammatical_complexity.clear();
		lexical_richness.clear();
		prestige.clear();
		attractiveness.clear();
		active_speakers_counts.clear();
		generations.clear();
		is_extinct.clear();
	}
};

// =================
// LanguageHandle - Handle to reference a language
// =================
struct LanguageHandle
{
	uint16_t index = 0;      // Index in the LanguageData arrays
	uint32_t generation = 0; // Generation for validity checking
	bool operator==(const LanguageHandle& other) const {
		return index == other.index && generation == other.generation;
	}
	bool operator!=(const LanguageHandle& other) const {
		return !(*this == other);
	}
	bool isValid() const {
		return generation != 0;
	}
	static LanguageHandle Invalid() {
		return LanguageHandle{ 0, 0 };
	}
};


