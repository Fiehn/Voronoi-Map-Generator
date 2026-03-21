#pragma once
#include "flecs.h"
#include "flecs_schema.h"
#include "HistoryData.hpp"
#include "Logger.h"
#include "util.hpp"

enum struct ContradictionType {
	DuplicateDomain, // Two deities share the same domain
	OpposingDomains, // A deity has domains that are oppositional
	ConflictingAxioms // A deity's domains conflict with the religion's axioms
};

struct DeityContradiction {
	flecs::entity religion;
	flecs::entity deity_object; // The deity that has the contradiction
	flecs::entity deity_subject; // If multiple deities involved, e.g., for duplicate domain
	ContradictionType type;

	DeityContradiction(
		flecs::entity rel,
		flecs::entity obj,
		ContradictionType t,
		flecs::entity subj = flecs::entity::null()
	)
		: religion(rel)
		, deity_object(obj)
		, type(t)
		, deity_subject(subj)
	{
	}

	bool solve(flecs::world& world, History& history, uint32_t current_tick);
};