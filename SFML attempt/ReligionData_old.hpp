#pragma once
#include <vector>
#include <string>
#include <cstdint>

// ==================
// Religion Handle - Safe index-based references to a religion
// ==================
struct ReligionHandle {
	uint16_t index = 0;			// Index in the religions array
	uint16_t generation = 0;	// Generation for validity checking
	bool isValid() const { return generation != 0; };
	static ReligionHandle Invalid() { return ReligionHandle{ 0, 0 }; };
	bool operator==(const ReligionHandle& other) const {
		return index == other.index && generation == other.generation;
	}
};

struct DeityHandle {
	uint16_t index = 0;			// Index in the deities array
	uint16_t generation = 0;	// Generation for validity checking
	bool isValid() const { return generation != 0; };
	static DeityHandle Invalid() { return DeityHandle{ 0, 0 }; };
	bool operator==(const DeityHandle& other) const {
		return index == other.index && generation == other.generation;
	}
};

// ==================
// Religion Component Types
// ==================
// Religions consist of memetic components that will evolve and mix over time
// 1. Traits (0-1) values for different aspects of the religion (defined in ReligionData)
// 2. Tenents - core beliefs that define the religion (identified by IDs)
// 3. Holy Cites - important locations for the religion (identified by cell IDs)
// 4. Pantheon - deities worshipped in the religion (identified by IDs)
// 5. Types of Religion - organized, animistic, philosophical, etc.
enum class ReligionType : uint8_t {
	Animistic,
	Philosophical,
	Mystical,
	Monotheistic,
	Polytheistic,
	Humanistic,
	Atheistic
};

// Tenents such as : Resource_restriction: "Pork", Proselytization: "High", Afterlife_belief: "Heaven"
// Ritaul wine, fasting, pilgrimage, etc.
// TODO: Create a structure for tenents

// The pantheon is defined by a procedural graph system
// each deity (node) has domains (e.g., war, fertility, wisdom)
// Each edge represents a relationship (e.g. parent-of, spouse-of, servant-of, enemy-of)
enum class DeityDomain : uint8_t {
	War,
	Fertility,
	Wisdom,
	Love,
	Nature,
	Sea,
	Sky,
	Forests,
	Underworld,
	Horses,
	Metals,
	Storms,
	Music,
	Trade,
	Wind,
	Fire,
	Death,
	Darkness,
	Sun,
	Travel,
	Moon,
	Order,
	Chaos,
	Mountains,
	Winter,
	Water,
	Law,

}; // TODO: Create more domains

enum class DeityRelationship : uint8_t {
	ParentOf,
	SpouseOf,
	ServantOf,
	EnemyOf,
	SiblingOf,
	SlayerOf,
	ProtectorOf,
	CreatorOf,
	AuthorOf,
	MutilatorOf
};

struct DeityBehaviorTree {
	// Placeholder for behavior tree structure
	// In a full implementation, this would define the deity's actions and interactions
};

// Archetypes based loosely on Wallace’s Typology of Religions and Jungian Archetypes
// e.g., The Creator, The Destroyer, The Trickster, The Protector, The Nurturer
// Will influence deity behaviors and relationships
struct DeityArchetypeComponent {
	std::string name;
	uint8_t id;
	DeityBehaviorTree behavior_tree; // Placeholder for behavior tree structure
};
struct Tenet {
	uint32_t id; // Unique identifier for the tenet
	std::string name; // Name of the tenet
	std::string description; // Description of the tenet
};

// ==================
// DeityData - Structure of Arrays for deity attributes
// ==================
struct DeityData {
	std::vector<std::string> names; // Names of deities
	std::vector<uint16_t> archetype_ids; // Archetype IDs for deities
	std::vector<uint16_t> source_religion_ids; // Religion ID that created the deity

	// === Attributes ===
	std::vector<std::vector<DeityDomain>> domains; // Domains for each deity
	std::vector<float> prominence_levels; // Prominence level of the deity (0-1)
	std::vector<float> benevolence_levels; // Benevolence level of the deity (0-1)

	// === Relationships ===
	std::vector<uint32_t> relationship_offsets; // Start index in shared relationship pool
	std::vector<uint32_t> relationship_counts;  // Number of relationships

	// === Metadata Management ===
	std::vector<uint16_t> generations; // Generation numbers for validity checking

	size_t size() const {
		return names.size();
	}
	void reserve(size_t n) {
		names.reserve(n);
		archetype_ids.reserve(n);
		source_religion_ids.reserve(n);
		domains.reserve(n);
		prominence_levels.reserve(n);
		benevolence_levels.reserve(n);
		relationship_offsets.reserve(n);
		relationship_counts.reserve(n);
		generations.reserve(n);
	}
	void resize(size_t n) {
		names.resize(n);
		archetype_ids.resize(n);
		source_religion_ids.resize(n);
		domains.resize(n);
		prominence_levels.resize(n);
		benevolence_levels.resize(n);
		relationship_offsets.resize(n);
		relationship_counts.resize(n);
		generations.resize(n);
	}
	void clear() {
		names.clear();
		archetype_ids.clear();
		source_religion_ids.clear();
		domains.clear();
		prominence_levels.clear();
		benevolence_levels.clear();
		relationship_offsets.clear();
		relationship_counts.clear();
		generations.clear();
	}
};

// ==================
// DeityRelationshipData - Structure of Arrays for deity relationships
// ==================
struct DeityRelationshipData {
	// Relationships
	std::vector<uint16_t> source_deity_ids; // Deity ID that is the source of the relationship
	std::vector<uint16_t> target_deity_ids; // Deity ID that is the target of the relationship
	std::vector<DeityRelationship> relationship_types; // Type of relationship
	std::vector<float> relationship_strengths; // Strength of the relationship (0-1)

	size_t size() const {
		return source_deity_ids.size();
	}
	void reserve(size_t n) {
		source_deity_ids.reserve(n);
		target_deity_ids.reserve(n);
		relationship_types.reserve(n);
		relationship_strengths.reserve(n);
	}
	void resize(size_t n) {
		source_deity_ids.resize(n);
		target_deity_ids.resize(n);
		relationship_types.resize(n);
		relationship_strengths.resize(n);
	}
	void clear() {
		source_deity_ids.clear();
		target_deity_ids.clear();
		relationship_types.clear();
		relationship_strengths.clear();
	}
};

// ==================
// Pantheon - A collection of deities worshipped in a religion
// ==================
struct Pantheon {
	std::vector<DeityHandle> deities; // List of deity handles in the pantheon
	uint16_t head_deity_index = 0; // Index of the head deity in the pantheon

	size_t size() const {
		return deities.size();
	}
	void clear() {
		deities.clear();
		head_deity_index = 0;
	}
	void reserve(size_t n) {
		deities.reserve(n);
	}
	void resize(size_t n) {
		deities.resize(n);
	}
};

// ==================
// Religion Definitions Data
// ==================
struct ReligionDefinitionsData {
	std::vector<Tenet> tenets; // Names of tenents
	std::vector<DeityArchetypeComponent> deity_archetypes; // Deity archetypes
	std::vector<std::vector<float>> domain_semantic_similarity; // Semantic similarity matrix for deity domains
	std::vector<DeityDomain> all_domains; // List of all possible deity domains
	std::vector<ReligionType> all_religion_types; // List of all possible religion types
	std::vector<DeityRelationship> all_deity_relationships; // List of all possible deity relationships
};

// ==================
// ReligionData - Structure of Arrays for religion attributes
// ==================
struct ReligionData
{
	// === Identity ===
	std::vector<std::string> names;			// Name of the religion
	std::vector<ReligionType> religion_types; // Type of the religion

	// === Heritage (Phylogenetic relationships) ===
	std::vector<uint16_t> parent_religion_id; // Parent religion (id of the religion), 0 = proto-religion
	std::vector<uint32_t> founding_dates;    // Date of founding (in simulation ticks)
	std::vector<size_t> origin_cell_ids;   // Cell where the religion originated
	std::vector<float> divergence_rates;    // Rate of divergence from parent religion (0-1)

	// === Religious Traits ===
	std::vector<float> proselytization_rate; // Rate of conversion efforts
	std::vector<float> ritual_complexity;    // Complexity of rituals
	std::vector<float> afterlife_belief_strength; // Strength of afterlife beliefs
	std::vector<float> moral_strictness;     // Strictness of moral codes
	std::vector<float> clergy_influence;     // Influence of religious leaders
	std::vector<float> community_focus;     // Focus on community vs individual
	std::vector<float> tolerance_level;     // Tolerance towards other religions
	std::vector<float> pacifism_level;     // Level of pacifism promoted

	// === Tenents ===
	std::vector<std::vector<uint32_t>> tenents; // List of tenent IDs defining core beliefs

	// === Holy Places ===
	std::vector<std::vector<size_t>> holy_city_cell_ids; // List of cell IDs for holy cities

	// === Pantheon ===
	std::vector<Pantheon> pantheons; // Pantheon for each religion

	// === Metadata Management ===
	std::vector<uint16_t> generations; // Generation numbers for validity checking

	size_t size() const {
		return names.size();
	}
	void reserve(size_t n) {
		names.reserve(n);
		religion_types.reserve(n);
		parent_religion_id.reserve(n);
		founding_dates.reserve(n);
		origin_cell_ids.reserve(n);
		divergence_rates.reserve(n);
		proselytization_rate.reserve(n);
		ritual_complexity.reserve(n);
		afterlife_belief_strength.reserve(n);
		moral_strictness.reserve(n);
		clergy_influence.reserve(n);
		community_focus.reserve(n);
		tolerance_level.reserve(n);
		pacifism_level.reserve(n);
		tenents.reserve(n);
		holy_city_cell_ids.reserve(n);
		pantheons.reserve(n);
		generations.reserve(n);
	}
	void resize(size_t n) {
		names.resize(n);
		religion_types.resize(n);
		parent_religion_id.resize(n);
		founding_dates.resize(n);
		origin_cell_ids.resize(n);
		divergence_rates.resize(n);
		proselytization_rate.resize(n);
		ritual_complexity.resize(n);
		afterlife_belief_strength.resize(n);
		moral_strictness.resize(n);
		clergy_influence.resize(n);
		community_focus.resize(n);
		tolerance_level.resize(n);
		pacifism_level.resize(n);
		tenents.resize(n);
		holy_city_cell_ids.resize(n);
		pantheons.resize(n);
		generations.resize(n);
	}
	void clear() {
		names.clear();
		religion_types.clear();
		parent_religion_id.clear();
		founding_dates.clear();
		origin_cell_ids.clear();
		divergence_rates.clear();
		proselytization_rate.clear();
		ritual_complexity.clear();
		afterlife_belief_strength.clear();
		moral_strictness.clear();
		clergy_influence.clear();
		community_focus.clear();
		tolerance_level.clear();
		pacifism_level.clear();
		tenents.clear();
		holy_city_cell_ids.clear();
		pantheons.clear();
		generations.clear();
	}

};