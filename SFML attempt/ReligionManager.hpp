#pragma once
#include "flecs.h"
#include "flecs_schema.h"
#include "HistoryData.hpp"
#include "cell.hpp"
#include "Voronoi.hpp"
#include "DeityContradiction.hpp"
#include "MythicRules.hpp"

class ReligionManager
{
public:
	ReligionManager() = default;

	void initialize(flecs::world& world);

	void create_proto_religion(flecs::world& world, History& history, Cell& cell);
	void derive_religion(flecs::world& world, flecs::entity parent_religion_entity, Cell& cell);

	void mark_extinct(flecs::entity religion_entity);

	// === Accessors ===


	// === Evolution Step ===
	void evolve(flecs::world& world, History& history, int32_t tick);

	// === Myth Phase ===
	void myth_phase_evolution(flecs::world& world, History& history, flecs::entity religion_entity, uint32_t ticks_in_phase, vor::Voronoi& map);

private:
	std::vector<MythicRule> m_all_rules;
	// === Data Loading Helpers ===
	void load_domain_data(flecs::world& world);
	void load_archetype_data(flecs::world& world);
	void load_religion_types(flecs::world& world);

	// === Proto Religion Generation Helpers ===
	std::string get_proto_religion_name();
	void determine_proto_type(flecs::world& world, flecs::entity religion_entity);
	void add_proto_axioms(flecs::world& world, flecs::entity religion_entity, Cell& cell, int nr_axioms);
	void determine_proto_stats(flecs::world& world, flecs::entity religion_entity, Cell& cell);
	void generate_proto_pantheon(flecs::world& world, History& history, Cell& cell, flecs::entity religion_entity, int nr_gods);
	
	// === Evolution Helpers ===
	std::vector<DeityContradiction> count_contradictions(flecs::world& world, flecs::entity religion_entity);
};