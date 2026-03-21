#pragma once
#include "flecs.h"
#include "Cell.hpp"
#include "HistoryData.hpp"

class CultureManager
{
public:
	CultureManager();
	void initialize(flecs::world& world);
	void create_proto_culture(flecs::world& world, Cell& cell);
	void derive_culture(flecs::world& world, flecs::entity parent_culture_entity, Cell& cell);
	void mark_extinct(flecs::entity culture_entity);
	// === Accessors ===

	// === Evolution Step ===
	void evolve(flecs::world& world, History& history, int32_t tick);
private:

};