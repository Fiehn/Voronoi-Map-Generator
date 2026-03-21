#pragma once
#include "MythicRules.hpp"
#include "Logger.h"
#include "util.hpp"

namespace MythicRules {
	MythicRule CreateSplitDeityRule()
	{
		MythicRule rule;
		rule.name = "Cosmic Mitosis";
		rule.max_ticks = 20; // cooldown of 20 ticks

		// Match: Find Chaos or Order domain gods with no relations
		rule.match = [](flecs::world& world, flecs::entity religion) -> std::optional<RuleCandidate> 
			{
				std::optional<RuleCandidate> best_cand;

				// Query: find chaos or order god with 0 connections
				world.query_builder<Deity>()
					.with<GodOf>(world.lookup("Chaos") || world.lookup("Order"))
					.with(flecs::ChildOf, religion)
					.build()
					.each([&](flecs::entity god, const Deity& _d) 
						{
							// check connections
							int connections = 0;
							god.each<SiblingOf>([&](flecs::entity) {connections++; });
							god.each<ParentOf>([&](flecs::entity) {connections++; });

							if (connections == 0)
							{
								// Found a lonely chaos or order god
								float score = 1.0f;
								best_cand = { god, flecs::entity::null()};
							}
						});
				return best_cand;
			};
		rule.apply = [](flecs::world& world,History& history, RuleCandidate cand)
			{
				// Split candidate into 2-3 gods with domains that have chaos or order domainTag
				int nr =  RandomBetweenInt(2, 3);

				// Get original god's domains
				std::vector<flecs::entity> original_domains;
				cand.subject.each<GodOf>([&](flecs::entity domain) 
					{
						original_domains.push_back(domain);
					});
				// find all domains with chaos or order as domainTags
				std::vector<flecs::entity> possible_domains;
				std::vector<float> domain_weights;
				for (auto& domain_tag : { "Chaos", "Order" })
				{
					world.query_builder<Domain>()
						.with<DomainTag>(world.lookup(domain_tag))
						.build()
						.each([&](flecs::entity domain, const Domain& _d) 
							{
								possible_domains.push_back(domain);
								domain_weights.push_back(1.0f); // equal weight for now
							});
				}
				// Remove chaos and order from original domains to avoid duplication
				original_domains.erase(
					std::remove_if(original_domains.begin(), original_domains.end(),
						[&](flecs::entity d)
						{
							return d.has<DomainTag>(world.lookup("Chaos")) || d.has<DomainTag>(world.lookup("Order"));
						}),
					original_domains.end());

				// Get religion entity
				flecs::entity religion;
				cand.subject.each<WorshippedBy>([&](flecs::entity rel) {
					religion = rel;
					});

				// Create new gods
				std::vector<flecs::entity> new_gods;
				for (int i = 0; i < nr; i++)
				{
					// Get name for new god
					std::string original_name = cand.subject.name().c_str();
					std::string new_god_name = original_name + "_Split_" + std::to_string(i + 1);

					flecs::entity new_god = world.entity(new_god_name.c_str())
						.add<Deity>()
						.add<WorshippedBy>(religion);
					// Get a random domain from possible_domains and 1 from original_domains
					if (!possible_domains.empty()) {
						int idx = chooseIndex(domain_weights);
						new_god.add<GodOf>(possible_domains[idx]);
						domain_weights[idx] = 0.0f; // remove this domain from future selection
					}

					if (!original_domains.empty()) {
						int idx = RandomBetweenInt(0, static_cast<int>(original_domains.size()) - 1);
						new_god.add<GodOf>(original_domains[idx]);
						original_domains.erase(original_domains.begin() + idx);
					}

					// Add splitFrom relation
					new_god.add<SplitFromDeity>(cand.subject);

					new_gods.push_back(new_god);
				}
				// Add sibling relations between new gods
				for (size_t i = 0; i < new_gods.size(); i++) {
					for (size_t j = i + 1; j < new_gods.size(); j++) {
						new_gods[i].add<SiblingOf>(new_gods[j]);
					}
				}
				
				// Mark original god extinct
				cand.subject.add<Extinct>();

				std::string subj_name = cand.subject.name().c_str() ? cand.subject.name().c_str() : "Unknown";

				HistoricalActor actor_subj(cand.subject);
				HistoricalActor actor_obj(cand.object);
				EventSnapshot snap(subj_name, "Split into smaller gods", "Cosmos", "Split gods");

				history.log(0, EventType::DeityInteraction, actor_subj, actor_obj, snap);

				LOG_INFO(Religion, "Deity ", subj_name, " split into ", nr, " new deities.");
			};
		rule.get_rule_weight = [](flecs::world& world, flecs::entity religion, vor::Voronoi& map) -> float
			{
				// Base weight because this is not dependent on location or other factors for now
				return 1.0f;
			};
		return rule;
	}

	MythicRule CreateMarriageRule()
	{
		MythicRule rule;
		rule.name = "Divine Union";
		rule.max_ticks = 5; // cooldown of 5 ticks

		// Match: Find two deities that are not married
		rule.match = [](flecs::world& world, flecs::entity religion) -> std::optional<RuleCandidate>
			{
				std::vector<flecs::entity> unmarried_candidates;

				// Query: find all deities of the religion
				world.each([&](flecs::entity deity, const Deity& _d)
					{
						if (deity.has<WorshippedBy>(religion))
						{
							bool is_married = false;
							deity.each<SpouseOf>([&](flecs::entity) { is_married = true; });
							if (!is_married)
							{
								unmarried_candidates.push_back(deity);
							}
						}
					});

				if (unmarried_candidates.size() >= 2) {
					int max_idx = static_cast<int>(unmarried_candidates.size()) - 1;
					int idx1 = RandomBetweenInt(0, max_idx);
					int idx2 = RandomBetweenInt(0, max_idx);
					while (idx1 == idx2) {
						idx2 = RandomBetweenInt(0, max_idx);
					}

					return RuleCandidate{ unmarried_candidates[idx1], unmarried_candidates[idx2] };
				}
				return std::nullopt;
			};
		
		// apply: Create marriage relation
		rule.apply = [](flecs::world& world, History& history, RuleCandidate cand)
			{
				cand.subject.add<SpouseOf>(cand.object);
				cand.object.add<SpouseOf>(cand.subject);

				std::string subj_name = cand.subject.name() ? cand.subject.name() : "Unknown";
				std::string obj_name = cand.object.name() ? cand.object.name() : "Unknown";

				LOG_INFO(Religion, "Deities ", subj_name, " and ", obj_name, " are now married.");

				HistoricalActor actor_subj(cand.subject);
				HistoricalActor actor_obj(cand.object);
				EventSnapshot snap(subj_name, "Married to " + obj_name, "Cosmos", "Divine Marriage");

				history.log(0, EventType::DeityInteraction, actor_subj, actor_obj, snap);
			};
		rule.get_rule_weight = [](flecs::world& world, flecs::entity religion, vor::Voronoi& map) -> float
			{
				float base_weight = 1.0f;

				const ReligionStats stats = religion.get<ReligionStats>();
				base_weight *= (1.0f + stats.community_focus);
				return base_weight;
			};
		return rule;
	}


	std::vector<MythicRule> GetAllRules()
	{
		return {
			CreateSplitDeityRule(),
			CreateMarriageRule()
		};
	}
}
