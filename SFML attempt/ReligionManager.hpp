#pragma once
#include "ReligionData.hpp"
#include "Voronoi.hpp"
#include "Logger.h"


// ====================
// Religion Create Info - Information needed to create a religion
// ====================
struct ReligionCreateInfo {
	std::string name;
	ReligionType religion_type;
	size_t origin_cell_id;
	uint32_t founding_date;
	// Optional - Parent religion
	uint16_t parent_religion = 0;
	// Initial values for religious traits
	float proselytization_rate = 0.5f; // Rate of conversion efforts
	float ritual_complexity = 0.5f;    // Complexity of rituals
	float afterlife_belief_strength = 0.5f; // Strength of afterlife beliefs
	float moral_strictness = 0.5f;     // Strictness of moral codes
	float clergy_influence = 0.5f;     // Influence of religious leaders
	float community_focus = 0.5f;     // Focus on community vs individual
	float tolerance_level = 0.5f;     // Tolerance towards other religions
	float pacifism_level = 0.5f;     // Level of pacifism promoted
	// Initial tenents (by IDs)
	std::vector<uint32_t> tenents;
	// Initial holy cities (by cell IDs)
	std::vector<size_t> holy_city_cell_ids;
};

// ====================
// Deity Create Info - Information needed to create a deity
// ====================
struct DeityCreateInfo {
	std::string name;
	uint16_t archetype_id;
	ReligionHandle source_religion;
	std::vector<DeityDomain> domains;
	float prominence_level = 0.5f; // Prominence level of the deity (0-1)
	float benevolence_level = 0.5f; // Benevolence level of the deity (0-1)
};

// ==================
// ReligionManager - Manages religions in the simulation
// ==================
class ReligionManager
{
public:
	ReligionManager();
	
	// === Religion Lifecycle ===
	ReligionHandle CreateReligion(const ReligionCreateInfo& info);
	ReligionHandle DeriveReligion(ReligionHandle parent_handle, const ReligionCreateInfo& info);
	void MarkExtinct(ReligionHandle handle);
	bool IsValid(ReligionHandle handle) const;

	// === Deity Lifecycle ===
	void AddDeityRelationship(DeityHandle source, DeityHandle target, DeityRelationship relationship_type, float strength = 1.0f);
	void RemoveDeityRelationship(DeityHandle source, DeityHandle target, DeityRelationship relationship_type);
	std::vector<DeityHandle> GetRelatedDeities(DeityHandle deity, DeityRelationship relationship_type) const;

	// === Pantheon Management ===
	void AddDeityToPantheon(ReligionHandle religion_handle, DeityHandle deity_handle);
	void RemoveDeityFromPantheon(ReligionHandle religion_handle, DeityHandle deity_handle);
	const Pantheon& GetPantheon(ReligionHandle religion_handle) const;
	std::vector<DeityHandle> GetPantheonDeities(ReligionHandle religion_handle) const;
	
	// === Pantheon Generation (public for testing) ===
	void GenerateProtoPantheon(ReligionHandle handle, Cell& origin_cell);

	// === Accessors ===
	ReligionData& GetReligionData() { return m_data; }
	const ReligionData& GetReligionData() const { return m_data; }
	DeityData& GetDeityData() { return m_deity_data; }
	const DeityData& GetDeityData() const { return m_deity_data; }
	DeityRelationshipData& GetDeityRelationshipData() { return m_deity_relationships; }
	const DeityRelationshipData& GetDeityRelationshipData() const { return m_deity_relationships; }
	ReligionDefinitionsData& GetReligionDefinitions() { return m_definitions; }
	const ReligionDefinitionsData& GetReligionDefinitions() const { return m_definitions; }

	std::string GetReligionName(ReligionHandle handle) const;
	std::string GetDeityName(DeityHandle handle) const;
	std::vector<size_t> GetHolyCities(ReligionHandle handle) const;

	// === Religious Dynamics ===
	void UpdateReligions(vor::Voronoi& voronoi, uint32_t current_tick);

private:
	ReligionData m_data;
	ReligionDefinitionsData m_definitions;
	DeityData m_deity_data;
	DeityRelationshipData m_deity_relationships;

	uint16_t m_next_generation = 1;
	uint16_t m_next_deity_generation = 1;

	// === Allocation Helpers ===
	uint16_t AllocateSlot();
	uint16_t AllocateDeitySlot();

	// === Pantheon Generation ===
	void DerivePantheon(ReligionHandle child_handle, ReligionHandle parent_handle, Cell& origin_cell_child);
	void Syncretism(ReligionHandle religon_a, ReligionHandle religion_b, Cell& contact_cell, float relative_strength);
	
	// === Deity Generation Helpers ===
	void GenerateDeityName(DeityHandle deity, ReligionHandle religion_handle);
	void GenerateDeityDomains(DeityHandle deity, const Cell& origin_cell);
	void GenerateDeityRelationships(DeityHandle deity, const std::vector<DeityHandle>& pantheon);
	void RepairDeityConflict(int deity_index);
	int CountDeityConflicts(ReligionHandle religion_handle, const Cell& origin_cell) const;
	DeityDomain GetRandomDomainExcluding(const std::vector<DeityDomain>& exclude_domains) const;

	// === Pantheon Operations ===
	void PantheonMerge(ReligionHandle target_religion, ReligionHandle source_religion);
	void TenetBlending(ReligionHandle target_religion, ReligionHandle source_religion);
	void CleanUpPantheon(ReligionHandle religion_handle);

	// === Religious Events ===
	void TriggerSchism(ReligionHandle parent_religion, Cell& schism_cell);
	void TriggerReformation(ReligionHandle religion_handle, Cell& reform_cell);
	void TriggerRevival(ReligionHandle religion_handle, Cell& revival_cell);
	void MutateTenet(ReligionHandle religion_handle);
	void MutateTraits(ReligionHandle religion_handle);
	void MutatePantheon(ReligionHandle religion_handle);
	void SpawnProphet(ReligionHandle religion_handle, Cell& origin_cell);
	void EstablishHolySite(ReligionHandle religion_handle, Cell& site_cell);
	void SpreadSect(ReligionHandle religion_handle, Cell& target_cell, float splitPower);

	// === Helpers ===
	void InitializeReligionDefinitions();
	float SemanticSimilarity(DeityDomain a, DeityDomain b) const;
	float SemanticInterceptionBetweenSects(const std::vector<DeityDomain>& set_a, const std::vector<DeityDomain>& set_b) const;
	std::vector<std::vector<float>> LoadSemanticSimilarityTable(const std::vector<DeityDomain>& all_domains);
};