#include "DeityContradiction.hpp"

bool DeityContradiction::solve(flecs::world& world, History& history, uint32_t current_tick)
{
	uint32_t tick = current_tick;

	switch (type)
	{
	case ContradictionType::DuplicateDomain: {
		// We need to find the shared domain
		flecs::entity shared_domain = flecs::entity::null();
		deity_object.each<GodOf>([&](flecs::entity domain) {
			if (deity_subject.has<GodOf>(domain))
			{
				shared_domain = domain;
			}
			});

		if (!shared_domain)
		{
			LOG_ERROR(Religion, "Failed to solve DuplicateDomain contradiction: no shared domain found between ", deity_object.name().c_str(), " and ", deity_subject.name().c_str());
			return false; // This should not happen, but just in case
		}
		// PATH 1: Divine War
		if (RandomBetween(0.0f, 1.0f) <= 0.7)
		{
			std::string reason = "";
			float rand_val = RandomBetween(0.0f, 1.0f);
			if (rand_val >= 0.9f)
			{
				reason = "A fight erupted over dominion of " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.8f)
			{
				reason = "Worshippers of " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + " clashed over control of " + std::string(shared_domain.name());

			}
			else if (rand_val >= 0.7f)
			{
				reason = "The Heavens were thrown into turmoil as " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + " vied for control over " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.6f)
			{
				reason = "The balance of power was disrupted as " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + " fought for supremacy over " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.5f)
			{
				reason = "The divine order was shaken as " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + " clashed over control of " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.4f)
			{
				reason = "Like a thief in the night, " + std::string(deity_object.name()) + " stole the " + std::string(shared_domain.name()) + " from " + std::string(deity_subject.name()) + ", sparking a divine conflict";
			}
			else if (rand_val >= 0.3f)
			{
				reason = "The heavens trembled as " + std::string(deity_object.name()) + " usurped " + std::string(deity_subject.name()) + "'s control over " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.2f)
			{
				reason = "The divine balance was upset as " + std::string(deity_object.name()) + " wrested control of " + std::string(shared_domain.name()) + " from " + std::string(deity_subject.name());
			}
			else
			{
				reason = "In a bold move, " + std::string(deity_object.name()) + " seized control of " + std::string(shared_domain.name()) + " from " + std::string(deity_subject.name()) + ", igniting a conflict in the heavens";
			}

			// Event A: The Crisis
			uint64_t crisis_id = history.log(tick, EventType::DivineConflict,
				HistoricalActor(deity_object), HistoricalActor(deity_subject),
				EventSnapshot(std::string(deity_object.name()), std::string(deity_subject.name()),
					"The Heavens", "A fight erupted over dominion of " + std::string(shared_domain.name())));

			// ECS Resolution
			deity_subject.remove<GodOf>(shared_domain);
			deity_subject.add<EnemyOf>(deity_object); // Mark the two deities as enemies

			// Event B: The Aftermath
			history.log(tick, EventType::DeityInteraction,
				HistoricalActor(deity_subject), HistoricalActor(shared_domain),
				EventSnapshot(std::string(deity_subject.name()), std::string(shared_domain.name()),
					"The Heavens", "Stripped of " + std::string(shared_domain.name()) + " by " + std::string(deity_object.name())),
				crisis_id); // Link to Event A
		}
		else // PATH 2: Syncretism
		{
			std::string reason = "";
			float rand_val = RandomBetween(0.0f, 1.0f);

			if (rand_val >= 0.9f)
			{
				reason = "Worshippers began confusing " + std::string(deity_object.name()) + " with " + std::string(deity_subject.name()) + " because of their shared domain of " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.8f)
			{
				reason = "The lines between " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + " blurred as worshippers couldn't tell them apart due to their shared domain of " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.7f)
			{
				reason = "In the eyes of worshippers, " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + " became indistinguishable as they both claimed dominion over " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.6f)
			{
				reason = "The shared domain of " + std::string(shared_domain.name()) + " caused worshippers to conflate the identities of " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name());
			}
			else if (rand_val >= 0.5f)
			{
				reason = "As worshippers struggled to differentiate between " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + ", the two deities began to merge in popular belief due to their shared domain of " + std::string(shared_domain.name());
			}
			else if (rand_val >= 0.4f)
			{
				reason = "The shared domain of " + std::string(shared_domain.name()) + " led to a blending of identities between " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name()) + ", as worshippers couldn't tell them apart";
			}
			else if (rand_val >= 0.3f)
			{
				reason = std::string(deity_object.name()) + "'s association with the shared domain of " + std::string(shared_domain.name()) + " caused worshippers to conflate it with " + std::string(deity_subject.name());
			}
			else if (rand_val >= 0.2f)
			{
				reason = "The shared domain of " + std::string(shared_domain.name()) + " caused confusion among worshippers, leading them to merge the identities of " + std::string(deity_object.name()) + " and " + std::string(deity_subject.name());
			}
			else
			{
				reason = "Worshippers began conflating " + std::string(deity_object.name()) + " with " + std::string(deity_subject.name()) + " due to their shared domain of " + std::string(shared_domain.name()) + ", leading to a blending of their identities";
			}

			// Event A: The Crisis
			uint64_t crisis_id = history.log(tick, EventType::Syncretism,
				HistoricalActor(deity_object), HistoricalActor(deity_subject),
				EventSnapshot(std::string(deity_object.name()), std::string(deity_subject.name()),
					"Mortal Minds, ", "Worshippers began confusing " + std::string(deity_object.name()) + " with " + std::string(deity_subject.name()) + " because of their shared domain of " + std::string(shared_domain.name()))
			);
			// ECS Resolution
			deity_subject.each<GodOf>([&](flecs::entity d)
				{
					deity_object.add<GodOf>(d); // The subject deity also gains all domains of the object deity
				});
			deity_subject.add<Extinct>(); // The subject deity becomes extinct as it is fully syncretized into the object deity

			// Event B: The Aftermath
			history.log(tick, EventType::DeityInteraction,
				HistoricalActor(deity_subject), HistoricalActor(deity_object),
				EventSnapshot(std::string(deity_subject.name()), std::string(deity_object.name()),
					"The Heavens", std::string(deity_subject.name()) + " was absorbed into " + std::string(deity_object.name()) + " by worshippers who couldn't tell them apart because of their shared domain of " + std::string(shared_domain.name())),
				crisis_id); // Link to Event A
		}
		return true;
	}
	case ContradictionType::OpposingDomains:
	{
		// Find the opposing domains
		flecs::entity dom_a, dom_b;
		deity_object.each<GodOf>([&](flecs::entity d1) {
			deity_object.each<GodOf>([&](flecs::entity d2) {
				if (d1 != d2 && d1.has<OpposedTo>(d2)) { dom_a = d1; dom_b = d2; }
				});
			});
		if (!dom_a || !dom_b)
		{
			LOG_ERROR(Religion, "Failed to solve OpposingDomains contradiction: no opposing domains found for ", deity_object.name().c_str());
			return false; // This should not happen, but just in case
		}

		// PATH 1: Schism / Mitosis
		if (RandomBetween(0.0f, 1.0f) <= 0.5f)
		{
			std::string reason = "";
			float rand_val = RandomBetween(0.0f, 1.0f);
			if (rand_val >= 0.9f)
			{
				reason = "Internal strife tore " + std::string(deity_object.name()) + " apart as followers clashed over the opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			else if (rand_val >= 0.8f)
			{
				reason = "The divine order was shaken as " + std::string(deity_object.name()) + " fractured under the weight of its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			else if (rand_val >= 0.7f)
			{
				reason = "The heavens were thrown into chaos as " + std::string(deity_object.name()) + " split into two over the conflict between its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			else if (rand_val >= 0.6f)
			{
				reason = "Like a house divided, " + std::string(deity_object.name()) + " was torn apart by internal conflict over its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			else if (rand_val >= 0.5f)
			{
				reason = "The balance of power was disrupted as " + std::string(deity_object.name()) + " fractured into two separate entities due to the conflict between its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			else if (rand_val >= 0.4f)
			{
				reason = std::string(deity_object.name()) + "'s internal contradictions came to a head as it split into two separate deities over the conflict between its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			else if (rand_val >= 0.3f)
			{
				reason = "The divine balance was upset as " + std::string(deity_object.name()) + "'s opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name()) + " caused it to fracture into two separate entities";
			}
			else if (rand_val >= 0.2f)
			{
				reason = "The heavens trembled as " + std::string(deity_object.name()) + " split into two separate deities due to the conflict between its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			else
			{
				reason = "In a cataclysmic event, " + std::string(deity_object.name()) + " fractured into two separate entities as its followers clashed over the opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name());
			}
			// Event A: The Crisis
			uint64_t crisis_id = history.log(tick, EventType::Schism,
				HistoricalActor(deity_object), HistoricalActor(religion),
				EventSnapshot(std::string(deity_object.name()), std::string(religion.name()),
					"The Heavens", reason));
			// ECS Resolution
			deity_object.remove<GodOf>(dom_b);

			std::string new_name = std::string(deity_object.name()) + "'s Shadow";
			auto new_deity = world.entity(new_name.c_str())
				.add<Deity>()
				.add<WorshippedBy>(religion)
				.add<GodOf>(dom_b)
				.add<EnemyOf>(deity_object) // They hate each other now?
				.add<SplitFromDeity>(deity_object);

			// Event B: The Aftermath
			history.log(tick, EventType::DeityInteraction,
				HistoricalActor(new_deity), HistoricalActor(deity_object),
				EventSnapshot(std::string(new_deity.name()), std::string(deity_object.name()),
					"The Heavens", std::string(new_deity.name()) + " emerged from the schism of " + std::string(deity_object.name()) + " due to the conflict between its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name())),
				crisis_id); // Link to Event A
		}
		else // PATH 2: Shedding
		{
			std::string reason = "";
			float rand_val = RandomBetween(0.0f, 1.0f);
			if (rand_val >= 0.8f)
			{
				reason = "Over time " + std::string(deity_object.name()) + " had come to be less associated with " + std::string(dom_a.name()) + " and more associated with " + std::string(dom_b.name()) + ", until it eventually shed its connection to " + std::string(dom_a.name()) + " entirely";
			}
			else if (rand_val >= 0.6f)
			{
				reason = "The balance of power shifted as " + std::string(deity_object.name()) + " gradually became more associated with " + std::string(dom_b.name()) + " and less associated with " + std::string(dom_a.name()) + ", until it eventually shed its connection to " + std::string(dom_a.name()) + " entirely";
			}
			else if (rand_val >= 0.4f)
			{
				reason = "Like a snake shedding its skin, " + std::string(deity_object.name()) + " gradually shed its association with " + std::string(dom_a.name()) + " as it became more closely tied to " + std::string(dom_b.name());
			}
			else if (rand_val >= 0.2f)
			{
				reason = "The divine order was reshaped as " + std::string(deity_object.name()) + " slowly lost its connection to " + std::string(dom_a.name()) + " and became more closely associated with " + std::string(dom_b.name());
			}
			else
			{
				reason = std::string(deity_object.name()) + "'s association with its opposing domains of " + std::string(dom_a.name()) + " and " + std::string(dom_b.name()) + " gradually shifted until it eventually shed its connection to " + std::string(dom_a.name()) + " entirely";
			}
			// Event A: The Crisis / Resolution (no separate aftermath since it's a gradual process)
			uint64_t crisis_id = history.log(tick, EventType::DeityInteraction,
				HistoricalActor(deity_object), HistoricalActor(religion),
				EventSnapshot(std::string(deity_object.name()), std::string(religion.name()),
					"The Heavens", reason));
			// ECS Resolution
			deity_object.remove<GodOf>(dom_a);
		}
		return true;
	}
	case ContradictionType::ConflictingAxioms: {
		// TODO: Implement this

		return true;
	}
	default:
		LOG_ERROR(Religion, "Unknown contradiction type for ", deity_object.name().c_str());
		return false;
	}

}