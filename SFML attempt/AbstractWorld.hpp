#pragma once
#include "Voronoi.hpp"
#include "HistoryData.hpp"
#include "flecs_schema.h"
#include "ReligionManager.hpp"
#include "LanguageManager.hpp"
#include "CultureManager.hpp"
#include <cstdint>
#include <string>
#include <vector>

class AbstractWorld {
	flecs::world world;
	History history;

	// Managers (stateless logic handlers)
	ReligionManager rel_mgr;
	LanguageManager lang_mgr;
	CultureManager cult_mgr;
public:
	void initialize() {
		// Load modules
		world.import<ReligionModule>();
		world.import<LanguageModule>();
		world.import<CultureModule>();
		world.import<GlobalModule>();
		// Initialize managers (will register components/systems as needed)
		rel_mgr.initialize(world);
		lang_mgr.initialize(world);
		cult_mgr.initialize(world);
	}

	void step(int tick) { // SHOULD INCLUDE VORONOI MAP AND POPULATION SYSTEM TO ACTUALLY MAKE THE EVOLUTIONS WORK
		
		lang_mgr.evolve(world, history, tick); // Language evolution is independent
		cult_mgr.evolve(world, history, tick); // Culture evolution depends primarily on language
		rel_mgr.evolve(world, history, tick); // Religion evolution depends on culture and language

		// I SHOULD DO A READ AND A WRITE PHASE PER MANAGER TO AVOID CONFLICTS (TODO)
		// e.g., first have all managers read the world and history to determine changes,
		// then have them apply those changes to the world and history.

		world.progress(); // Advance the ECS world (I have to figure out how to use this properly)
	}

	void myth_phase_step(flecs::entity religion_entity, uint32_t tick, vor::Voronoi& map) {
		// This is the myth phase or pre-history phase evolution step
		// This is the first age after initial point generation
		// Where the pops flood out and religions establish myths and resolve contradictions
		// Language and culture may also have myth phase evolutions, but religion is primary here
		rel_mgr.myth_phase_evolution(world, history, religion_entity, tick, map);
	}
};



