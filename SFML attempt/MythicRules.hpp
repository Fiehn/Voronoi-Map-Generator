#pragma once
#include "flecs.h"
#include "flecs_schema.h"
#include "HistoryData.hpp"
#include "Voronoi.hpp"
#include <vector>
#include <string>
#include <optional>
#include <functional>

// ===============
// Mythic Rule System
// ===============
struct RuleCandidate {
	flecs::entity subject; // e.g., religion entity
	flecs::entity object;  // e.g., deity entity

};

struct MythicRule {
	std::string name;

	// 1. Check if the rule can run and return a "Candidate" (the entities involved)
	std::function<std::optional<RuleCandidate>(flecs::world& world, flecs::entity religion)> match;

	// 2. Execute the rule
	std::function<void(flecs::world& world, History& history, RuleCandidate candidate)> apply;

	// 3. Get weight for candidate
	std::function<float(flecs::world& world, flecs::entity religion, vor::Voronoi& map)> get_rule_weight;

	// cooldown
	int cooldown_ticks = 0; // current cooldown
	int max_ticks = 0; // set to >0 to enable cooldown
};

// ===============
// Mythic Rule functions collection 
// ===============
namespace MythicRules {
	MythicRule CreateSplitDeityRule();
	MythicRule CreateMergeDeitiesRule();
	MythicRule CreateContradictionResolutionRule();
	MythicRule CreateEnemyRule();
	MythicRule CreateServantRule();
	MythicRule CreateParentChildRule();
	
	std::vector<MythicRule> GetAllRules();
}
