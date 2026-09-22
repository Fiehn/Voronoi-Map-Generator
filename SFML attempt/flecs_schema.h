#pragma once
#include "flecs.h"
#include <string>
#include <vector>
#include <array>

// ==================
// 1. Religion Components
// ==================

// TODO: Figure out how to represent the deity behavior trees in flecs or just in general
// TODO: Figure out the Tenet system, structure, and representation

// Flecs for Religion System
struct ReligionStats {
	float proselytization;  // 0.0 - 1.0 (1 is proselytize aggressively)	// Use for conversion efforts
	float ritual_complexity; // 0.0 - 1.0 (1 is highly complex rituals)		// Use for behavioral tree complexity
	float moral_strictness;  // 0.0 - 1.0 (1 is very strict)				// Use for tension if behavior deviates (resources not sacrificed, leads to larger unrest)
	float community_focus;   // 0.0 - 1.0 (1 is highly community-focused)	// Use for social cohesion
	float tolerance;        // 0.0 - 1.0 (1 is very tolerant)				// Use for inter-group relations
	float pacifism;        // 0.0 - 1.0 (1 is very pacifist)				// Use for willingness to war
	float fervor;          // 0.0 - 1.0 (1 is very fervent)					// Use for zeal in rituals and proselytization
};

// Each religion generates 2-5 axioms that define its worldview and how domains interact
// eg. Natural is hostile to Artificial leading to domains with Natural affinity opposing those with Artificial affinity
struct AxiomRules {
	struct Rule {
		flecs::entity tag_a; 	 // e.g., "Natural"
		flecs::entity tag_b; 	 // e.g., "Artificial"
		bool is_hostile;         // true = "Natural is hostile to Artificial"
		std::string description; // e.g., "Artificial is an affront to the natural order"
	};
	std::vector<Rule> rules;
};

struct SemanticAxes {
	// SHOULD BE std::array<float, N>, but I need to check how many axes I want first
	// Thermai (heat), -1 = Ice, 0 = Temperate, 1 = Fire
	// Baric (Density), -1 = Ethereal, 0 = Normal, 1 = Solid
	// Lumic (Light), -1 = Dark, 0 = twillight, 1 = Radiant
	// Vital (Life), -1 = Death, 0 = construct/inert , 1 = Life
	// Entropic (Chaos), -1 = Order, 0 = Neutral, 1 = Chaos
	// VAD model ( https://github.com/EmilHvitfeldt/textdata/blob/main/R/lexicon_nrc_vad.R )
	// Valence, -1 = Negative, 0 = Neutral, 1 = Positive
	// Arousal, -1 = Calm, 0 = Neutral, 1 = Excited
	// Dominance, -1 = Submissive, 0 = Neutral, 1 = Dominant
	std::array<float, 8> values; // array of 8 floats representing the axes
};

struct Archetype { };
struct HasArchetype {}; // Deity -> Archetype
struct ArchetypeRules {
	float conflict_tendency; // 0.0 - 1.0 (1 is very conflict-prone)
	std::vector<flecs::entity> preferred_domains; // Domains preferred by this archetype
};
struct ArchetypeIndex { int32_t index; }; // To bitmask archetypes and ensure unique assignment

// Tags 
struct Religion {};	// Tag for a religion entity
struct ReligionType {}; // Tag for religion types (animism, polytheism, monotheism, etc)
struct Deity {}; // Tag for a deity entity
struct Domain {}; // Tag for a domain entity
struct DomainTag {}; // Tag for domain properties (artificial, natural, liminal, etc)

// Relationships
struct WorshippedBy {}; // Religion -> Deity
struct GodOf {}; // Deity -> Domain
struct OpposedTo {}; // Domain -> Domain
struct HasTag {}; // Domain -> DomainTag
struct SplitFromDeity {}; // Deity -> Deity


// pantheon relationships
struct SpouseOf {}; // Deity -> Deity
struct ParentOf {}; // Deity -> Deity
struct MutilatorOf {}; // Deity -> Deity
struct EnemyOf {}; // Deity -> Deity
struct ServantOf {}; // Deity -> Deity
struct ChildOf {}; // Deity -> Deity
struct SiblingOf {}; // Deity -> Deity

// Module
struct ReligionModule {
	ReligionModule(flecs::world& world) {
		world.module<ReligionModule>();

		// Register data
		world.component<ReligionStats>();
		world.component<AxiomRules>();
		world.component<ArchetypeRules>();
		world.component<SemanticAxes>();

		// Register tags
		world.component<Religion>();
		world.component<ReligionType>();
		world.component<Deity>();
		world.component<Domain>();
		world.component<DomainTag>();
		world.component<Archetype>();

		// Register relationships
		world.component<HasArchetype>().add(flecs::Exclusive);
		world.component<WorshippedBy>();
		world.component<GodOf>();
		world.component<OpposedTo>().add(flecs::Symmetric);
		world.component<HasTag>();
		world.component<SpouseOf>();
		world.component<ParentOf>();
		world.component<MutilatorOf>();
		world.component<EnemyOf>();
		world.component<ServantOf>();
		world.component<ChildOf>();
		world.component<SiblingOf>().add(flecs::Symmetric);
		world.component<SplitFromDeity>();
	}
};


// ==================
// 2. Culture Components
// ==================
struct CultureRateStats { // all stats range from 0.0 to 1.0
	float innovation;    // (1 is highly innovative) used for tech adoption rate
	float military_tradition; // (1 is highly militaristic) used for military effectiveness
	float individualism; // (1 is highly individualistic) used for ?
	float materialism;   // (1 is highly materialistic) used for economic development
	float patriarchy;    // (1 is highly patriarchal) used for gender roles
	float temperance;    // (1 is highly temperate) used for social stability
	float closeness_to_nature; // (1 is very close to nature) used for environmental adaptation
	float religiosity;   // (1 is highly religious) used for religious adherence
	float art_focus;     // (1 is highly focused on arts) used for cultural development
	float social_stratification; // (1 is highly stratified) used for social hierarchy
	float expansionism;  // (1 is highly expansionist) used for territorial growth
	float trade_openness; // (1 is highly open to trade) used for economic prosperity
	float multiculturalism; // (1 is highly multicultural) used for social cohesion
	float naval_capability; // (1 is highly capable) used for naval expansion
	float mountain_adaptability; // (1 is highly adaptable) used for mountain region expansion
};

struct PrimaryLanguage {}; // Culture -> Language
struct PrimaryReligion {}; // Culture -> Religion

struct CultureFamily {}; // tag for culture families
struct Culture {}; // tag for culture entities

struct CultureModule {
	CultureModule(flecs::world& world)
	{
		world.module<CultureModule>();

		// Register data
		world.component<CultureRateStats>();

		// Register tags
		world.component<CultureFamily>();
		world.component<Culture>();

		// Register relationships
		world.component<PrimaryLanguage>();
		world.component<PrimaryReligion>();
	}
};

// =================
// 3. Language Components
// =================

struct LanguageStats {
	float phoneme_variability; // 0.0 - 1.0 (1 is highly variable)		// Use for phonetic drift
	float grammar_complexity;  // 0.0 - 1.0 (1 is highly complex)		// Use for syntactic drift
	float lexical_richness;    // 0.0 - 1.0 (1 is very rich)			// Use for vocabulary size
	float regularity;         // 0.0 - 1.0 (1 is very regular)			// Use for ease of learning
};

struct Language {}; // tag for language entities

struct LanguagePhonology {
	std::vector<uint16_t> phoneme_indices; // Index in IPA database
};

struct LanguageLexicon {
	struct Word {
		std::string phonetics;
		int32_t meaning_id; // Link to meaning database
	};
	std::vector<Word> words;
};

struct DerivedFrom {}; // Language -> Language
struct OriginatedFrom {}; // Language -> Culture
struct LiturgicalLanguageOf {}; // Language -> Religion

struct LanguageModule {
	LanguageModule(flecs::world& world)
	{
		world.module<LanguageModule>();
		// Register data
		world.component<LanguageStats>();
		world.component<LanguagePhonology>();
		world.component<LanguageLexicon>();
		// Register tags
		world.component<Language>();
		// Register relationships
		world.component<DerivedFrom>();
		world.component<OriginatedFrom>();
		world.component<LiturgicalLanguageOf>();
	}
};

// ==================
// 4. Global Module
// ==================
struct GeographicalOrigin {
	size_t cell_id; // Cell where the entity originated
};
struct PopulationOrigin {
	size_t pop_id; // Find the population id where the entity originated (SoA index) 
};
struct Extinct {}; // Tag for extinct entities

struct GlobalModule {
	GlobalModule(flecs::world& world)
	{
		world.module<GlobalModule>();
		// Register data
		world.component<GeographicalOrigin>();
		world.component<PopulationOrigin>();
		world.component<Extinct>();
	}
};