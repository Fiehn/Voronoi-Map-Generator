#pragma once
#include "LanguageManager.hpp"

// =================
// IPA Database Implementation
// =================
// === Helper Functions ===
// trims whitespace
static std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\n\r");
	if (first == std::string::npos) return "";
	size_t last = str.find_last_not_of(" \t\n\r");
	return str.substr(first, (last - first + 1));
}
// parse enum from string
static LanVoicing parseVoicing(const std::string& str) {
	if (str == "voiced") return LanVoicing::Voiced;
	if (str == "voiceless") return LanVoicing::Voiceless;
	return LanVoicing::NA;
}
static LanPlace parsePlace(const std::string& str) {
	if (str == "bilabial") return LanPlace::Bilabial;
	if (str == "labiodental") return LanPlace::Labiodental;
	if (str == "dental") return LanPlace::Dental;
	if (str == "alveolar") return LanPlace::Alveolar;
	if (str == "postalveolar") return LanPlace::Postalveolar;
	if (str == "retroflex") return LanPlace::Retroflex;
	if (str == "palatal") return LanPlace::Palatal;
	if (str == "velar") return LanPlace::Velar;
	if (str == "uvular") return LanPlace::Uvular;
	if (str == "pharyngeal") return LanPlace::Pharyngeal;
	if (str == "glottal") return LanPlace::Glottal;
	return LanPlace::NA;
}
static LanManner parseManner(const std::string& str) {
	if (str == "plosive") return LanManner::Plosive;
	if (str == "nasal") return LanManner::Nasal;
	if (str == "trill") return LanManner::Trill;
	if (str == "tap") return LanManner::Tap;
	if (str == "fricative") return LanManner::Fricative;
	if (str == "lateral_fricative") return LanManner::LateralFricative;
	if (str == "approximant") return LanManner::Approximant;
	if (str == "lateral_approximant") return LanManner::LateralApproximant;
	return LanManner::NA;
}
static LanHeight parseHeight(const std::string& str) {
	if (str == "close") return LanHeight::Close;
	if (str == "near_close") return LanHeight::NearClose;
	if (str == "close_mid") return LanHeight::CloseMid;
	if (str == "mid") return LanHeight::Mid;
	if (str == "open_mid") return LanHeight::OpenMid;
	if (str == "near_open") return LanHeight::NearOpen;
	if (str == "open") return LanHeight::Open;
	return LanHeight::NA;
}
static LanBackness parseBackness(const std::string& str) {
	if (str == "front") return LanBackness::Front;
	if (str == "central") return LanBackness::Central;
	if (str == "back") return LanBackness::Back;
	return LanBackness::NA;
}
static LanRounding parseRounding(const std::string& str) {
	if (str == "rounded") return LanRounding::Rounded;
	if (str == "unrounded") return LanRounding::Unrounded;
	return LanRounding::NA;
}
// === Data Base Loading ===
bool IPADatabase::LoadFromCSV(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) {
		LOG_WARNING(Language, "Failure to load database, not able to open IPA Phoneme file.", filepath);
		return false;
	}
	all_phonemes.clear();
	symbol_to_index.clear();
	vowel_indices.clear();
	consonant_indices.clear();

	std::string line;
	// Skip header
	std::getline(file, line);

	while (std::getline(file, line)) {
		std::stringstream ss(line);
		std::string symbol, type_str, voicing_str, place_str, manner_str, height_str, backness_str, rounding_str;
		
		std::getline(ss, symbol, ',');
		std::getline(ss, type_str, ',');
		std::getline(ss, voicing_str, ',');
		std::getline(ss, place_str, ',');
		std::getline(ss, manner_str, ',');
		std::getline(ss, height_str, ',');
		std::getline(ss, backness_str, ',');
		std::getline(ss, rounding_str, ',');

		// Trim whitespace
		symbol = trim(symbol);
		type_str = trim(type_str);
		voicing_str = trim(voicing_str);
		place_str = trim(place_str);
		manner_str = trim(manner_str);
		height_str = trim(height_str);
		backness_str = trim(backness_str);
		rounding_str = trim(rounding_str);

		Phoneme phoneme;
		strncpy_s(phoneme.symbol, sizeof(phoneme.symbol), symbol.c_str(), _TRUNCATE); // TODO: WINDOWS specific
		phoneme.symbol[sizeof(phoneme.symbol) - 1] = '\0'; // Ensure null-termination

		phoneme.type = (type_str == "vowel") ? PhenomeType::Vowel : PhenomeType::Consonant;
		phoneme.voicing = parseVoicing(voicing_str);
		phoneme.place = parsePlace(place_str);
		phoneme.manner = parseManner(manner_str);
		phoneme.height = parseHeight(height_str);
		phoneme.backness = parseBackness(backness_str);
		phoneme.rounding = parseRounding(rounding_str);

		size_t index = all_phonemes.size();
		all_phonemes.push_back(phoneme);
		symbol_to_index[phoneme.symbol] = index;
		if (phoneme.IsVowel()) {
			vowel_indices.push_back(index);
		} else if (phoneme.IsConsonant()) {
			consonant_indices.push_back(index);
		}
	}
	LOG_INFO(Language, "Loaded IPA Database with {} phonemes from {}", all_phonemes.size(), filepath);
	return !all_phonemes.empty();
}

const Phoneme* IPADatabase::GetPhonemeBySymbol(const std::string& symbol) const
{
	auto it = symbol_to_index.find(symbol);
	if (it != symbol_to_index.end()) {
		return &all_phonemes[it->second];
	}
	return nullptr;
}

// =================
// LanguageManager Implementation
// =================

bool LanguageManager::InitializeIPADatabase(const std::string& filepath)
{
	return m_ipa_database.LoadFromCSV(filepath);
}

// === Fast Lexicon Access ===
const LexiconEntry* LanguageManager::GetWord(LanguageHandle handle, uint32_t word_id) const
{
	if (!IsValid(handle)) {
		return nullptr; // Invalid handle
	}

	// Try Cache first
	auto cache_it = m_language_caches.find(handle.index);
	if (cache_it != m_language_caches.end())
	{
		const auto& cache = *cache_it->second;
		auto lex_it = cache.word_id_to_local_idx.find(word_id);
		if (lex_it != cache.word_id_to_local_idx.end())
		{
			uint32_t local_idx = lex_it->second;
			return &cache.lexicon_entries[local_idx];
		}
		return nullptr; // Word not found in cache
	}
	// Fallback to linear search in pool
	uint32_t offset = m_data.lexicon_offsets[handle.index];
	uint32_t count = m_data.lexicon_counts[handle.index];
	for (uint32_t i = 0; i < count; ++i) {
		const LexiconEntry& entry = m_resource_pools.lexicon_pool[offset + i];
		if (entry.word_id == word_id) {
			return &entry;
		}
	}
	return nullptr; // Word not found
}

std::string LanguageManager::GetPhoneticForm(LanguageHandle handle, uint32_t word_id) const
{
	const LexiconEntry* entry = GetWord(handle, word_id);
	if (entry) {
		return entry->phonetic_form;
	}
	return ""; // Word not found
}

// === Lazy Cache Building ===
const LanguageCache& LanguageManager::GetOrCreateLanguageCache(LanguageHandle handle)
{
	auto it = m_language_caches.find(handle.index);
	if (it != m_language_caches.end()) {
		return *it->second;
	}
	// Build Cache
	auto cache = std::make_unique<LanguageCache>();
	BuildCache(handle.index, *cache);

	auto result = cache.get();
	m_language_caches[handle.index] = std::move(cache);
	return *result;
}

void LanguageManager::BuildCache(uint16_t lang_idx, LanguageCache& out_cache)
{
	out_cache.language_id = lang_idx;

	// Phoneme Inventory
	uint32_t phoneme_offset = m_data.phoneme_inventory_offsets[lang_idx];
	uint32_t phoneme_count = m_data.phoneme_inventory_counts[lang_idx];
	out_cache.phonemes = &m_resource_pools.phoneme_pool[phoneme_offset];

	// Lexicon Entries
	uint32_t lexicon_offset = m_data.lexicon_offsets[lang_idx];
	uint32_t lexicon_count = m_data.lexicon_counts[lang_idx];
	out_cache.lexicon_entries = &m_resource_pools.lexicon_pool[lexicon_offset];

	// build word id to local index map
	for (uint32_t i = 0; i < lexicon_count; ++i) {
		const LexiconEntry& entry = m_resource_pools.lexicon_pool[lexicon_offset + i];
		out_cache.word_id_to_local_idx[entry.word_id] = i;
	}
	// Sound Change Rules
	uint32_t sound_change_offset = m_data.sound_change_offsets[lang_idx];
	uint32_t sound_change_count = m_data.sound_change_counts[lang_idx];
	out_cache.sound_changes = &m_resource_pools.sound_change_pool[sound_change_offset];

	// Syllable Template
	out_cache.syllable_template = m_data.syllable_templates[lang_idx];
}

// === Name Generation ===
std::string LanguageManager::GenerateName(LanguageHandle handle, int syllable_count)
{
	if (!IsValid(handle)) {
		return ""; // Invalid handle
	}

	const auto& cache = GetOrCreateLanguageCache(handle);
	const SyllableTemplate& syllable_template = cache.syllable_template;

	std::string name;

	for (int syl = 0; syl < syllable_count; ++syl) {
		// Onset (consonant before vowel)
		int onset_size = RandomBetweenInt(syllable_template.onset_min, syllable_template.onset_max);

		for (int i = 0; i < onset_size; ++i)
		{
			// Pick Random consonant from phenome inventory
			std::vector<uint8_t> consonants;
			for (uint16_t p = 0; p < cache.phoneme_count; ++p)
			{
				if (cache.phonemes[p].type == PhenomeType::Consonant) {
					consonants.push_back(p);
				}
			}
			if (!consonants.empty()) {
				uint8_t phoneme_idx = consonants[RandomBetweenInt(0, static_cast<int>(consonants.size()))];
				name += cache.phonemes[phoneme_idx].symbol;
			}
		}

		// Nucleus (vowel)
		std::vector<uint8_t> vowels;
		for (uint16_t p = 0; p < cache.phoneme_count; ++p)
		{
			if (cache.phonemes[p].type == PhenomeType::Vowel) {
				vowels.push_back(p);
			}
		}
		if (!vowels.empty()) {
			uint8_t phoneme_idx = vowels[RandomBetweenInt(0, static_cast<int>(vowels.size()))];
			name += cache.phonemes[phoneme_idx].symbol;
		}
		// Coda (consonant after vowel)
		int coda_size = RandomBetweenInt(syllable_template.coda_min, syllable_template.coda_max);
		for (int i = 0; i < coda_size; ++i)
		{
			// Pick Random consonant from phenome inventory
			std::vector<uint8_t> consonants;
			for (uint16_t p = 0; p < cache.phoneme_count; ++p)
			{
				if (cache.phonemes[p].type == PhenomeType::Consonant) {
					consonants.push_back(p);
				}
			}
			if (!consonants.empty()) {
				uint8_t phoneme_idx = consonants[RandomBetweenInt(0, static_cast<int>(consonants.size()))];
				name += cache.phonemes[phoneme_idx].symbol;
			}
		}
	}
	// Capitalize first letter
	if (!name.empty()) {
		name[0] = static_cast<char>(std::toupper(name[0]));
	}
	return name;
}

// === Sound Change Application ===
void LanguageManager::ApplySoundChanges(LanguageHandle handle, const std::vector<SoundChangeRule>& new_rules)
{
	if (!IsValid(handle)) {
		return; // Invalid handle
	}
	uint16_t lang_idx = handle.index;

	// Add rules to pool
	uint32_t rule_offset = m_resource_pools.sound_change_pool.size();
	m_resource_pools.sound_change_pool.insert(
		m_resource_pools.sound_change_pool.end(),
		new_rules.begin(),
		new_rules.end()
	);

	// Update Language metadata
	m_data.sound_change_offsets[lang_idx] = rule_offset;
	m_data.sound_change_counts[lang_idx] = static_cast<uint32_t>(new_rules.size());

	// Apply rules to lexicon
	uint32_t lexicon_offset = m_data.lexicon_offsets[lang_idx];
	uint32_t lexicon_count = m_data.lexicon_counts[lang_idx];

	const Phoneme* phonemes;
	uint16_t phoneme_count;
	phonemes = GetPhonemeInventory(handle, phoneme_count);

	// Apply each rule to each word
	for (const auto& rule : new_rules)
	{
		for (uint32_t i = 0; i < lexicon_count; ++i)
		{
			LexiconEntry& entry = m_resource_pools.lexicon_pool[lexicon_offset + i];

			// Apply rule to entry's phonetic form with probability
			if (RandomBetween(0.0f, 1.0f) <= rule.probability)
			{
				entry.phonetic_form = ApplyRuleToWord(entry.phonetic_form, rule, phonemes, phoneme_count);
			}
		}
	}

	ClearCache(handle);
}


// === Managers ===
LanguageManager::LanguageManager()
{
	// Reserve initial capacity
	m_data.reserve(256);
}

LanguageHandle LanguageManager::CreateProtoLanguage(const std::string& name,
	size_t origin_cell_id,
	int biome_type)
{
	// Allocate slot
	uint16_t lang_idx = AllocateSlot();
	// Initialize Language Data
	m_data.names[lang_idx] = name;
	m_data.parent_language_id[lang_idx] = 0; // No parent
	m_data.origin_cell_ids[lang_idx] = origin_cell_id;
	m_data.divergence_dates[lang_idx] = 0;
	m_data.divergence_scores[lang_idx] = 0.0f;
	m_data.generations[lang_idx] = m_next_generation++;
	m_data.is_extinct[lang_idx] = false;
	// Generate Phoneme Inventory
	GeneratePhonemeInventory(lang_idx, biome_type);
	// Generate Proto Lexicon
	GenerateProtoLexicon(lang_idx);
	// Initialize Linguistic Traits
	m_data.phonetic_complexity[lang_idx] = RandomBetween(0.3f, 0.7f);
	m_data.grammatical_complexity[lang_idx] = RandomBetween(0.3f, 0.7f);
	m_data.lexical_richness[lang_idx] = RandomBetween(0.3f, 0.7f);
	m_data.prestige[lang_idx] = RandomBetween(0.0f, 1.0f);
	m_data.attractiveness[lang_idx] = RandomBetween(0.0f, 1.0f);
	m_data.active_speakers_counts[lang_idx] = 100; // Initial speakers
	return LanguageHandle{ lang_idx, m_data.generations[lang_idx] };
}

LanguageHandle LanguageManager::DeriveLanguage(LanguageHandle parent_handle,
	const std::string& name,
	size_t origin_cell_id,
	const std::vector<SoundChangeRule>& divergence_rules)
{
	if (!IsValid(parent_handle)) {
		return LanguageHandle::Invalid(); // Invalid parent
	}
	uint16_t parent_idx = parent_handle.index;
	// Allocate slot
	uint16_t child_idx = AllocateSlot();
	// Initialize Language Data
	m_data.names[child_idx] = name;
	m_data.parent_language_id[child_idx] = parent_idx;
	m_data.origin_cell_ids[child_idx] = origin_cell_id;
	m_data.divergence_dates[child_idx] = 0; // Set to current simulation date as needed
	m_data.divergence_scores[child_idx] = 0.0f; // Will be updated over time
	m_data.generations[child_idx] = m_next_generation++;
	m_data.is_extinct[child_idx] = false;
	// Add to parent's derived languages
	m_data.derived_languages[parent_idx].push_back(child_idx);
	// Inherit Phoneme Inventory
	InheritPhonemeInventory(child_idx, parent_idx);
	// Derive Lexicon
	DeriveLexicon(child_idx, parent_idx);
	// Apply Sound Changes
	ApplySoundChanges(LanguageHandle{ child_idx, m_data.generations[child_idx] }, divergence_rules);
	// Initialize Linguistic Traits with some variation
	m_data.phonetic_complexity[child_idx] = std::clamp(
		m_data.phonetic_complexity[parent_idx] + RandomBetween(-0.1f, 0.1f), 0.0f, 1.0f);
	m_data.grammatical_complexity[child_idx] = std::clamp(
		m_data.grammatical_complexity[parent_idx] + RandomBetween(-0.1f, 0.1f), 0.0f, 1.0f);
	m_data.lexical_richness[child_idx] = std::clamp(
		m_data.lexical_richness[parent_idx] + RandomBetween(-0.1f, 0.1f), 0.0f, 1.0f);
	m_data.prestige[child_idx] = std::clamp(
		m_data.prestige[parent_idx] + RandomBetween(-0.1f, 0.1f), 0.0f, 1.0f);
	m_data.attractiveness[child_idx] = std::clamp(
		m_data.attractiveness[parent_idx] + RandomBetween(-0.1f, 0.1f), 0.0f, 1.0f);
	m_data.active_speakers_counts[child_idx] = 50; // Initial speakers
	return LanguageHandle{ child_idx, m_data.generations[child_idx] };
}

void LanguageManager::MarkExtinct(LanguageHandle handle)
{
	if (!IsValid(handle)) {
		return; // Invalid handle
	}
	uint16_t index = handle.index;
	// Mark as extinct
	m_data.is_extinct[index] = true;
	//Clear active speakers
	m_data.active_speakers_counts[index] = 0;
	// Remove cache
	auto cache_it = m_language_caches.find(index);
	if (cache_it != m_language_caches.end()) {
		m_language_caches.erase(cache_it);
	}
}
bool LanguageManager::IsExtinct(LanguageHandle handle) const
{
	if (!IsValid(handle)) {
		return true; // Invalid handles are considered "extinct"
	}
	return m_data.is_extinct[handle.index];
}

size_t LanguageManager::GetActiveSpeakers(LanguageHandle handle) const
{
	if (!IsValid(handle) || IsExtinct(handle)) {
		return 0;
	}
	return m_data.active_speakers_counts[handle.index];
}

bool LanguageManager::IsValid(LanguageHandle handle) const
{
	if (handle.index >= m_data.size()) {
		return false;
	}
	// Check generation for validity
	return m_data.generations[handle.index] == handle.generation;
}

// === Phenome Access ===
const Phoneme* LanguageManager::GetPhonemeInventory(LanguageHandle handle, uint16_t& out_count) const
{
	if (!IsValid(handle)) {
		out_count = 0;
		return nullptr; // Invalid handle
	}
	uint16_t lang_idx = handle.index;
	uint32_t offset = m_data.phoneme_inventory_offsets[lang_idx];
	uint32_t count = m_data.phoneme_inventory_counts[lang_idx];
	out_count = static_cast<uint16_t>(count);
	return &m_resource_pools.phoneme_pool[offset];
}

// === Sound Change Application Helpers ===
void LanguageManager::ApplySingleRule(LanguageHandle handle, const SoundChangeRule& rule)
{
	if (!IsValid(handle)) {
		return; // Invalid handle
	}
	std::vector<SoundChangeRule> rules = { rule };
	ApplySoundChanges(handle, rules);
}

// === Phylogenetic Queries ===
float LanguageManager::CalculateLinguisticDistance(LanguageHandle a, LanguageHandle b) const
{
	if (!IsValid(a) || !IsValid(b)) {
		return 1.0f; // Max distance for invalid languages
	}

	if (a.index == b.index) {
		return 0.0f; // Same language
	}

	// Calculate lexical similarity
	uint32_t a_lex_offset = m_data.lexicon_offsets[a.index];
	uint32_t a_lex_count = m_data.lexicon_counts[a.index];
	uint32_t b_lex_offset = m_data.lexicon_offsets[b.index];
	uint32_t b_lex_count = m_data.lexicon_counts[b.index];

	float total_distance = 0.0f;
	int compared_words = 0;

	uint32_t words_to_compare = std::min(std::min(a_lex_count, b_lex_count), (uint32_t)50);

	for (uint32_t i = 0; i < words_to_compare; ++i)
	{
		const LexiconEntry& a_entry = m_resource_pools.lexicon_pool[a_lex_offset + i];
		const LexiconEntry& b_entry = m_resource_pools.lexicon_pool[b_lex_offset + i];

		if (a_entry.word_id == b_entry.word_id) {
			float word_distance = CalculateLevenshteinDistance(a_entry.phonetic_form, b_entry.phonetic_form);
			total_distance += word_distance;
			compared_words++;
		}
	}

	if (compared_words == 0) {
		return 1.0f; // No comparable words, max distance
	}

	// normalize distance
	float avg_distance = total_distance / compared_words;
	return clamp(avg_distance, 0.0f, 1.0f);
}
LanguageHandle LanguageManager::GetCommonAncestor(LanguageHandle a, LanguageHandle b) const
{
	if (!IsValid(a) || !IsValid(b)) {
		return LanguageHandle::Invalid(); // Invalid handles
	}
	if (a.index == b.index) {
		return a; // Same language
	}

	// Build ancestry chains for a
	std::vector<uint16_t> a_ancestry;
	uint16_t current = a.index;
	while (current != 0) {
		a_ancestry.push_back(current);
		current = m_data.parent_language_id[current];

		if (a_ancestry.size() > 100) {
			break; // Prevent infinite loops
		}
	}
	if (current == 0) {
		a_ancestry.push_back(0); // Add proto-language
	}
	// Traverse b's ancestry to find common ancestor
	current = b.index;
	int iterations = 0;
	while (current != 0 && iterations < 100)
	{
		// Check if current is in a's ancestry
		for (uint16_t ancestor : a_ancestry) {
			if (ancestor == current) {
				return LanguageHandle{ current, m_data.generations[current] };
			}
		}
		current = m_data.parent_language_id[current];
		iterations++;
	}
	if (current == 0) {
		return LanguageHandle{ 0, m_data.generations[0] }; // Return proto-language
	}
	return LanguageHandle::Invalid(); // No common ancestor found
}
float LanguageManager::CalculateMutualIntelligibility(LanguageHandle a, LanguageHandle b) const
{
	if (!IsValid(a) || !IsValid(b)) {
		return 0.0f; // No intelligibility for invalid languages
	}
	if (a.index == b.index) {
		return 1.0f; // Full intelligibility for same language
	}
	// Calculate linguistic distance
	float distance = CalculateLinguisticDistance(a, b);

	// Find common ancestor and calculate divergence time
	LanguageHandle ancestor = GetCommonAncestor(a, b);
	float time_factor = 0.0f;

	if (ancestor.isValid()) {
		uint32_t a_divergence = m_data.divergence_dates[a.index];
		uint32_t b_divergence = m_data.divergence_dates[b.index];
		uint32_t ancestor_divergence = m_data.divergence_dates[ancestor.index];
		uint32_t divergence_time_a = a_divergence - ancestor_divergence;
		uint32_t divergence_time_b = b_divergence - ancestor_divergence;

		// Simple model: intelligibility decreases with distance and divergence time
		time_factor = static_cast<float>(divergence_time_a + divergence_time_b) / 10000.0f; // Normalize TODO: check again
	}

	float intelligibility = (1.0f - distance) * std::exp(-time_factor);
	return clamp(intelligibility, 0.0f, 1.0f);
}
// === Cache Management ===
void LanguageManager::ClearCache(LanguageHandle handle)
{
	if (!IsValid(handle)) {
		return; // Invalid handle
	}
	auto cache_it = m_language_caches.find(handle.index);
	if (cache_it != m_language_caches.end()) {
		m_language_caches.erase(cache_it);
	}
}

void LanguageManager::ClearAllCaches()
{
	m_language_caches.clear();
}

// === Place name generation ===
std::string LanguageManager::GeneratePlaceName(LanguageHandle handle,
	const Cell& cell,
	bool include_semantic_meaning)
{
	if (!IsValid(handle)) {
		return ""; // Invalid handle
	}

	std::string name = GenerateName(handle, RandomBetweenInt(2, 4));

	if (include_semantic_meaning) {
		// Append semantic meaning based on cell properties TODO: improve
		std::string suffix;

		if (cell.height > 0.9f) {
			suffix = GenerateName(handle, 1); // Mountain-like suffix
		}
		else if (cell.oceanBool && cell.height < 0.2f) {
			suffix = GenerateName(handle, 1); // deep Ocean-like suffix
		}
		else if (cell.riverBool) {
			suffix = GenerateName(handle, 1); // River-like suffix
		}
		else if (cell.oceanBool) {
			suffix = GenerateName(handle, 1); // Generic ocean-like suffix
		}
		else {
			suffix = GenerateName(handle, 1); // Generic land-like suffix
		}

		name = name + suffix;
	}
	return name;
}

// === Word Generation === TODO: improve
std::string LanguageManager::GenerateWord(LanguageHandle handle,
	uint32_t word_id,
	SemanticCategory category) const
{
	if (!IsValid(handle)) {
		return ""; // Invalid handle
	}

	// Check if word exists
	const LexiconEntry* entry = GetWord(handle, word_id);
	if (entry) {
		return entry->phonetic_form;
	}

	// Generate new word
	uint16_t phoneme_count;
	const Phoneme* phonemes = GetPhonemeInventory(handle, phoneme_count);

	if (!phonemes || phoneme_count == 0) {
		return ""; // No phonemes available
	}

	// Generate syllables
	int syllable_count = RandomBetweenInt(1, 5);
	std::string word;

	for (int i = 0; i < syllable_count; ++i)
	{
		// Add Consonant
		if (RandomBetween(0.0f, 1.0f) > 0.5f)
		{
			int consonant_idx = RandomBetweenInt(0, phoneme_count);
			while (phonemes[consonant_idx].type != PhenomeType::Consonant) {
				consonant_idx = RandomBetweenInt(0, phoneme_count);
			}
			word += phonemes[consonant_idx].symbol;
		}
		// Add Vowel
		for (int attempt = 0; attempt < 10; ++attempt)
		{
			int vowel_idx = RandomBetweenInt(0, phoneme_count);
			if (phonemes[vowel_idx].type == PhenomeType::Vowel) {
				word += phonemes[vowel_idx].symbol;
				break;
			}
		}
	}
	return word;
}

// === Internal Helpers ===
uint16_t LanguageManager::AllocateSlot()
{
	if (!m_free_list.empty()) {
		uint16_t index = m_free_list.back();
		m_free_list.pop_back();
		return index;
	}

	// Allocate new slot
	uint16_t new_index = static_cast<uint16_t>(m_data.size());
	m_data.resize(new_index + 1);
	return new_index;
}

void LanguageManager::FreeSlot(uint16_t index)
{
	// Mark as extinct but don't actually free (for historical preservation)
	m_data.is_extinct[index] = true;
	m_data.active_speakers_counts[index] = 0;

	// Clear cache
	auto it = m_language_caches.find(index);
	if (it != m_language_caches.end()) {
		m_language_caches.erase(it);
	}
}

// === Phoneme Generation ===
void LanguageManager::GeneratePhonemeInventory(uint16_t lang_idx, int biome_type)
{
	// biome is ignoered for now TODO: LATER
	// Generate a phoneme inventory based on syngergy rules
	std::vector<Phoneme> phonemes;

	// vowels (as per basic rules of conlang we want at least 3 vowels and max 7, they need to be fairly distinct and
	// and they should cover front, central and back positions) While also considering height (high, mid, low)
	int vowel_count = RandomBetweenInt(3, 7);
	std::vector<size_t> possible_vowels = m_ipa_database.vowel_indices;

	std::shuffle(possible_vowels.begin(), possible_vowels.end(), std::mt19937{ std::random_device{}() });
	for (int i = 0; i < vowel_count && i < possible_vowels.size(); ++i) {
		phonemes.push_back(m_ipa_database.all_phonemes[possible_vowels[i]]);
	}
	// consonants (we want at least 5 consonants and max 15, covering various places and manners of articulation)
	int consonant_count = RandomBetweenInt(5, 15);
	std::vector<size_t> possible_consonants = m_ipa_database.consonant_indices;

	std::shuffle(possible_consonants.begin(), possible_consonants.end(), std::mt19937{ std::random_device{}() });
	for (int i = 0; i < consonant_count && i < possible_consonants.size(); ++i) {
		phonemes.push_back(m_ipa_database.all_phonemes[possible_consonants[i]]);
	}
	// Store in resource pool
	uint32_t offset = static_cast<uint32_t>(m_resource_pools.phoneme_pool.size());
	m_resource_pools.phoneme_pool.insert(
		m_resource_pools.phoneme_pool.end(),
		phonemes.begin(),
		phonemes.end()
	);
	// Update Language metadata
	m_data.phoneme_inventory_offsets[lang_idx] = offset;
	m_data.phoneme_inventory_counts[lang_idx] = static_cast<uint32_t>(phonemes.size());
}

void LanguageManager::InheritPhonemeInventory(uint16_t child_idx, uint16_t parent_idx)
{
	// TODO: improve
	uint32_t parent_offset = m_data.phoneme_inventory_offsets[parent_idx];
	uint32_t parent_count = m_data.phoneme_inventory_counts[parent_idx];

	uint32_t child_offset = static_cast<uint32_t>(m_resource_pools.phoneme_pool.size());
	m_resource_pools.phoneme_pool.insert(
		m_resource_pools.phoneme_pool.end(),
		m_resource_pools.phoneme_pool.begin() + parent_offset,
		m_resource_pools.phoneme_pool.begin() + parent_offset + parent_count
	);

	// Update Language metadata
	m_data.phoneme_inventory_offsets[child_idx] = child_offset;
	m_data.phoneme_inventory_counts[child_idx] = parent_count;
}
// === Lexicon Generation ===
void LanguageManager::GenerateProtoLexicon(uint16_t lang_idx)
{
	// TODO : improve (use semantic categories, frequency distributions, etc.)
	std::vector<LexiconEntry> lexicon;
	// Generate basic vocabulary (100 words)
	for (uint32_t i = 0; i < 100; ++i) {
		LexiconEntry entry;
		entry.word_id = i;
		entry.phonetic_form = GenerateWord(
			LanguageHandle{ lang_idx, m_data.generations[lang_idx] },
			i,
			SemanticCategory::SC_NATURE
		);
		entry.semantic_category = static_cast<uint32_t>(SemanticCategory::SC_NATURE);
		entry.frequency_rank = static_cast<uint16_t>(i);
		entry.parent_language_id = 0;
		entry.parent_word_id = 0;
		entry.is_borrowed = false;
		entry.is_compound = false;

		lexicon.push_back(entry);
	}

	// Store in pool
	uint32_t offset = m_resource_pools.lexicon_pool.size();
	m_resource_pools.lexicon_pool.insert(
		m_resource_pools.lexicon_pool.end(),
		lexicon.begin(),
		lexicon.end()
	);

	m_data.lexicon_offsets[lang_idx] = offset;
	m_data.lexicon_counts[lang_idx] = static_cast<uint32_t>(lexicon.size());
}

void LanguageManager::DeriveLexicon(uint16_t child_idx, uint16_t parent_idx)
{
	// TODO: improve (add borrowing, innovation, etc.)
	// Copy parent's lexicon
	uint32_t parent_offset = m_data.lexicon_offsets[parent_idx];
	uint32_t parent_count = m_data.lexicon_counts[parent_idx];
	std::vector<LexiconEntry> child_lexicon;
	for (uint32_t i = 0; i < parent_count; ++i)
	{
		const LexiconEntry& parent_entry = m_resource_pools.lexicon_pool[parent_offset + i];
		LexiconEntry child_entry = parent_entry;
		// Update parent references
		child_entry.parent_language_id = parent_idx;
		child_entry.parent_word_id = parent_entry.word_id;
		child_lexicon.push_back(child_entry);
	}
	// Store in pool
	uint32_t child_offset = m_resource_pools.lexicon_pool.size();
	m_resource_pools.lexicon_pool.insert(
		m_resource_pools.lexicon_pool.end(),
		child_lexicon.begin(),
		child_lexicon.end()
	);
	m_data.lexicon_offsets[child_idx] = child_offset;
	m_data.lexicon_counts[child_idx] = static_cast<uint32_t>(child_lexicon.size());
}

// === Sound Change Application Helper ===
std::string LanguageManager::ApplyRuleToWord(const std::string& word,
	const SoundChangeRule& rule,
	const Phoneme* phonemes,
	uint16_t phoneme_count) const
{
	if (phoneme_count == 0 || word.empty()) {
		return word;
	}

	// Simple implementation: replace target phoneme with replacement
	std::string result = word;

	const char* target_symbol = phonemes[rule.target_phoneme_index].symbol;
	const char* replacement_symbol = phonemes[rule.replacement_phoneme_index].symbol;

	size_t pos = 0;
	while ((pos = result.find(target_symbol, pos)) != std::string::npos) {
		result.replace(pos, strlen(target_symbol), replacement_symbol);
		pos += strlen(replacement_symbol);
	}

	return result;
}

// === Levenshtein Distance Calculation ===
float LanguageManager::CalculateLevenshteinDistance(const std::string & a,
	const std::string & b) const
{
	if (a.empty()) return static_cast<float>(b.length());
	if (b.empty()) return static_cast<float>(a.length());

	std::vector<std::vector<int>> dp(a.length() + 1, std::vector<int>(b.length() + 1));

	for (size_t i = 0; i <= a.length(); ++i) {
		dp[i][0] = static_cast<int>(i);
	}
	for (size_t j = 0; j <= b.length(); ++j) {
		dp[0][j] = static_cast<int>(j);
	}

	for (size_t i = 1; i <= a.length(); ++i) {
		for (size_t j = 1; j <= b.length(); ++j) {
			int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
			dp[i][j] = std::min({
				dp[i - 1][j] + 1,      // deletion
				dp[i][j - 1] + 1,      // insertion
				dp[i - 1][j - 1] + cost // substitution
				});
		}
	}

	// Normalize to 0-1 range
	float max_len = static_cast<float>(std::max(a.length(), b.length()));
	return dp[a.length()][b.length()] / max_len;
}

bool LanguageManager::ValidateHandle(LanguageHandle handle) const
{
	return handle.index < m_data.size() &&
		m_data.generations[handle.index] == handle.generation &&
		handle.generation != 0;
}