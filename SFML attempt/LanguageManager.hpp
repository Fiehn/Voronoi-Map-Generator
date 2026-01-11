#pragma once
#include "LanguageData.hpp"
#include "Cell.hpp"
#include <unordered_map>
#include <memory>
#include "util.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "Logger.h"

// Global IPA Phoneme List for reference
struct IPADatabase {
	std::vector<Phoneme> all_phonemes;
	std::unordered_map<std::string, size_t> symbol_to_index;

	std::vector<size_t> vowel_indices;
	std::vector<size_t> consonant_indices;

	bool LoadFromCSV(const std::string& filepath);
	const Phoneme* GetPhonemeBySymbol(const std::string& symbol) const;
};

// =================
// Shared Resource Pools - Memory efficiency
// =================
class LanguageResourcePools
{
public:
	// Shared Phoneme Pool
	std::vector<Phoneme> phoneme_pool;
	// Shared Sound Change Rules Pool
	std::vector<SoundChangeRule> sound_change_pool;
	// Shared Lexicon Pool
	std::vector<LexiconEntry> lexicon_pool;

	// Fast lookup: word_id -> phonetic forms in different languages
	std::unordered_map<uint32_t, std::vector<uint32_t>> word_id_to_lexicon_indices;

	// Phenome Lookup for fast parsing
	std::unordered_map<std::string, uint8_t> phoneme_symbol_to_index;
};

// =================
// Language Cache - Hot data for active languages
// =================
struct LanguageCache {
	uint16_t language_id = 0;

	// Frequently accessed data cached contiguously
	const Phoneme* phonemes = nullptr; // Pointer to phoneme inventory
	uint16_t phoneme_count = 0;

	const LexiconEntry* lexicon_entries = nullptr; // Pointer to lexicon entries
	uint32_t lexicon_count = 0;

	const SoundChangeRule* sound_changes = nullptr; // Pointer to sound change rules
	uint32_t sound_change_count = 0;

	SyllableTemplate syllable_template{};

	// Quick Lookup
	std::unordered_map<uint32_t, uint32_t> word_id_to_local_idx;
};

// =================
// LanguageManager - Manages creation and storage of languages
// =================
class LanguageManager
{
public:
	LanguageManager();

	// === IPA Database Loading ===
	bool InitializeIPADatabase(const std::string& filepath = "Assets/IPA/IPA_phonemes.csv");
	const IPADatabase& GetIPADatabase() const { return m_ipa_database; }

	// === Language Lifecycle ===
	LanguageHandle CreateProtoLanguage(const std::string& name, 
		size_t origin_cell_id, 
		int biome_type);
	LanguageHandle DeriveLanguage(LanguageHandle parent_handle, 
		const std::string& name, 
		size_t origin_cell_id,
		const std::vector<SoundChangeRule>& divergence_rules);
	void MarkExtinct(LanguageHandle handle);
	bool IsValid(LanguageHandle handle) const;

	// === Accessors ===
	LanguageData& GetLanguageData() { return m_data; }
	const LanguageData& GetLanguageData() const { return m_data; }

	LanguageResourcePools& GetResourcePools() { return m_resource_pools; }
	const LanguageResourcePools& GetResourcePools() const { return m_resource_pools; }

	// === Fast Lexicon Access ===
	const LexiconEntry* GetWord(LanguageHandle handle, uint32_t word_id) const;

	std::string GetPhoneticForm(LanguageHandle handle, uint32_t word_id) const;

	std::string GenerateWord(LanguageHandle handle,
		uint32_t word_id,
		SemanticCategory category) const;

	// === Phenome Access ===
	const Phoneme* GetPhonemeInventory(LanguageHandle handle, uint16_t& out_count) const;

	// === Sound Change Application ===
	void ApplySoundChanges(LanguageHandle handle, const std::vector<SoundChangeRule>& new_rules);

	void ApplySingleRule(LanguageHandle handle, const SoundChangeRule& rule);

	// === Phylogenetic Queries ===
	float CalculateLinguisticDistance(LanguageHandle a, LanguageHandle b) const;

	LanguageHandle GetCommonAncestor(LanguageHandle a, LanguageHandle b) const;

	float CalculateMutualIntelligibility(LanguageHandle a, LanguageHandle b) const;

	// === Cache Management ===
	const LanguageCache& GetOrCreateLanguageCache(LanguageHandle handle);

	void ClearCache(LanguageHandle handle);
	void ClearAllCaches();

	// === Name Generation ===
	std::string GenerateName(LanguageHandle handle, int syllable_count);
	std::string GeneratePlaceName(LanguageHandle handle,
		const Cell& cell,
		bool include_semantic_meaning = true);

	// === Statistics ===
	size_t GetTotalLanguages() const {
		return m_data.size();
	};
	size_t GetActiveLanguages() const {
		return m_language_caches.size();
	};

	// === Extinction Status ===
	bool IsExtinct(LanguageHandle handle) const;
	size_t GetActiveSpeakers(LanguageHandle handle) const;

private:
	LanguageData m_data;
	LanguageResourcePools m_resource_pools;
	IPADatabase m_ipa_database;

	// Cache for hot languages
	std::unordered_map<uint16_t, std::unique_ptr<LanguageCache>> m_language_caches;

	// Proto-lexicon: universal semantic ids
	std::vector<uint32_t> m_universal_word_ids;

	// Free list 
	std::vector<uint16_t> m_free_list;
	uint16_t m_next_generation = 1;

	// === Internal Helpers ===
	uint16_t AllocateSlot();
	void FreeSlot(uint16_t index);

	// Phoneme generation
	void GeneratePhonemeInventory(uint16_t lang_idx, int biome_type);
	void InheritPhonemeInventory(uint16_t child_idx, uint16_t parent_idx);

	// Lexicon generation
	void GenerateProtoLexicon(uint16_t lang_idx);
	void DeriveLexicon(uint16_t child_idx, uint16_t parent_idx);

	// Sound change simulation
	std::string ApplyRuleToWord(const std::string& word,
		const SoundChangeRule& rule,
		const Phoneme* phonemes,
		uint16_t phoneme_count) const;

	// Distance calculation
	float CalculateLevenshteinDistance(const std::string& a,
		const std::string& b) const;

	// Cache building
	void BuildCache(uint16_t lang_idx, LanguageCache& out_cache);

	bool ValidateHandle(LanguageHandle handle) const;
};