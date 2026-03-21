#include "ReligionManager.hpp"
#include "Logger.h"
#include "Include/nlohmann/json.hpp"
using json = nlohmann::json;
#include <fstream>
#include <string>
#include "flecs_schema.h"
#include "util.hpp"
#include "MythicRules.hpp"
#include "DeityContradiction.hpp"


void ReligionManager::load_domain_data(flecs::world& world)
{
	// Load domain data to ECS world from the JSON file: "Assets/Religions/domains.json"
	// Use nlohmann::json to parse the file and create entities with Domain tag and properties

	std::ifstream file("Assets/Religions/domains.json");
	if (!file.is_open())
	{
		LOG_ERROR(Religion, "Failed to open domains.json file, check assets.");
		return;
	}

	json data;
	file >> data;
	file.close();

	// Iterate through the JSON array and create domain entities
	for (auto& [domain_name, domain_data] : data.items())
	{
		// Create entity with domain tag
		auto domain_entity = world.entity(domain_name.c_str())
			.add<Domain>();

		// Add semantic axes
		if (domain_data.contains("semantic_vector"))
		{
			SemanticAxes axes = {};
			auto& vec = domain_data["semantic_vector"];

			// Map JSON keys to array indices (based on your schema comments)
			axes.values[0] = vec.value("thermai", 0.0f);
			axes.values[1] = vec.value("baric", 0.0f);
			axes.values[2] = vec.value("lumic", 0.0f);
			axes.values[3] = vec.value("vital", 0.0f);
			axes.values[4] = vec.value("entropic", 0.0f);
			axes.values[5] = vec.value("valence", 0.0f);
			axes.values[6] = vec.value("arousal", 0.0f);
			axes.values[7] = vec.value("dominance", 0.0f);

			domain_entity.set(axes);
		}
		else { LOG_WARNING(Religion, "Failed to find semantic axes for domain: ", domain_name.c_str()); }

		// Add domain tags if present in JSON
		if (domain_data.contains("domain_tags"))
		{
			for (const auto& tag : domain_data["domain_tags"])
			{
				auto tag_entity = world.entity(tag.get<std::string>().c_str())
					.add<DomainTag>();
				domain_entity.add<HasTag>(tag_entity);
			}
		}
		else { LOG_WARNING(Religion, "No domain tags present for domain: ", domain_name.c_str()); }
	}
	LOG_INFO(Religion, "Loaded domain data from Assets/Religions/domains.json into the world.");
}

void ReligionManager::load_archetype_data(flecs::world& world)
{
	// Load archetype data from JSON file: "Assets/Religions/archetypes.json"

	std::ifstream file("Assets/Religions/archetypes.json");
	if (!file.is_open())
	{
		LOG_ERROR(Religion, "Failed to open archetypes.json file, check assets.");
		return;
	}

	json data;
	file >> data;
	file.close();

	// Access the array
	if (!data.contains("archetypes") || !data["archetypes"].is_array())
	{
		LOG_ERROR(Religion, "Archetypes JSON is not an array.");
		return;
	}

	// Iterate through the archetypes
	for (const auto& archetype_data : data["archetypes"])
	{
		auto archetype_entity = world.entity(archetype_data["name"].get<std::string>().c_str())
			.add<Archetype>();

		// Add archetype ID
		if (archetype_data.contains("id"))
		{
			ArchetypeIndex idx;
			idx.index = archetype_data["id"].get<int32_t>();
			archetype_entity.set(idx);
		}
		// TODO: load favored domains 
		// TODO: load behavor trees here
	}
	LOG_INFO(Religion, "Loaded archetype data form Assets/Religions/archetypes.json");
}

void ReligionManager::initialize(flecs::world& world)
{
	// Load in data from external files
	load_domain_data(world);
	load_archetype_data(world);
	load_religion_types(world);
}
void ReligionManager::load_religion_types(flecs::world& world)
{
	world.entity("Animistic").add<ReligionType>();
	world.entity("Humanistic").add<ReligionType>();
	world.entity("Monotheistic").add<ReligionType>();
	world.entity("Polytheistic").add<ReligionType>();
}

void ReligionManager::create_proto_religion(flecs::world& world, History& history, Cell& cell)
{
	// The religion is created by:
	// 1. Determine type
	// 2. Determine religion Axioms
	// 3. Determine initial stats
	// 4. if a religion with gods:
	//	a. create 3-6 deities (using 1-3 domains)
	// 5. Then contradicitons are resolved in the myth phase


	// Create religion entity
	std::string name = get_proto_religion_name();

	auto religion_entity = world.entity(name.c_str())
		.add<Religion>();

	// === 1. Determine type ===
	determine_proto_type(world, religion_entity);
	
	// === 2. Determine religion Axioms ===
	int nr_axioms = RandomBetweenInt(2, 4);

	add_proto_axioms(world, religion_entity, cell, nr_axioms);

	// === 3. Determine initial stats ===
	determine_proto_stats(world, religion_entity, cell);

	// === 4. Grammar Loop ===
	// if religion type not humanism or atheist
	bool has_gods = religion_entity.has<ReligionType>(world.entity("Polytheistic")) ||
		religion_entity.has<ReligionType>(world.entity("Monotheistic")) ||
		religion_entity.has<ReligionType>(world.entity("Animistic"));

	if (has_gods)
	{
		int nr_proto_gods = RandomBetweenInt(3, 6);
		generate_proto_pantheon(world, history, cell, religion_entity, nr_proto_gods);
	}

	LOG_INFO(Religion, "Created Proto Religion ", name.c_str(), "has gods: ", has_gods);
}

std::string ReligionManager::get_proto_religion_name()
{
	// TODO: Implement name generation logic with language integration
	std::vector<std::string> possible_syllables = { "an", "or", "el", "ra", "zu", "ma", "ti", "ka", "lo", "vi" };

	int syllable_count = RandomBetweenInt(2, 4);
	std::string name;
	for (int i = 0; i < syllable_count; i++) {
		size_t index = static_cast<size_t>(RandomBetweenInt(0, static_cast<int>(possible_syllables.size()) - 1));
		name += possible_syllables[index];
	}
	return name;
}

void ReligionManager::determine_proto_type(flecs::world& world, flecs::entity religion_entity)
{
	// Get all religionType values from the world
	auto query = world.query<ReligionType>();

	std::vector<flecs::entity> religion_types;
	std::vector<float> weights;

	query.each([&](flecs::entity type_entity, const ReligionType& t) {
		religion_types.push_back(type_entity);

		// Assign weights based on type name
		std::string type_name = type_entity.name().c_str();

		if (type_name == "Animistic")
			weights.push_back(0.5f);
		else if (type_name == "Polytheistic")
			weights.push_back(0.48f);
		else if (type_name == "Monotheistic")
			weights.push_back(0.1f);
		else if (type_name == "Humanistic")
			weights.push_back(0.01f);
		else
			weights.push_back(0.01f);
		});
	if (religion_types.empty())
	{
		LOG_ERROR(Religion, "No religion types found in world!");
		return;
	}

	int index = chooseIndex(weights);

	religion_entity.add<ReligionType>(religion_types[index]);
	LOG_INFO(Religion, "Assigned Religion type: ", religion_types[index].name().c_str());
}

void ReligionManager::add_proto_axioms(flecs::world& world, flecs::entity religion_entity, Cell& cell, int nr_axioms)
{
	AxiomRules axioms;

	// Get all domain tags to create axioms between them
	auto query = world.query<DomainTag>();
	std::vector<flecs::entity> domain_tags;

	query.each([&](flecs::entity tag_entity, const DomainTag& tag) {
		domain_tags.push_back(tag_entity);
		});

	// Generate random axioms between the hidden tags
	for (int i = 0; i < nr_axioms && domain_tags.size() >= 2; i++)
	{
		// Pick random tags
		int idx_a = RandomBetweenInt(0, (int)domain_tags.size() - 1);
		int idx_b = RandomBetweenInt(0, (int)domain_tags.size() - 1);
		while (idx_a == idx_b)
		{
			idx_b = RandomBetweenInt(0, (int)domain_tags.size() - 1);
		}

		AxiomRules::Rule rule;
		rule.tag_a = domain_tags[idx_a];
		rule.tag_b = domain_tags[idx_b];
		rule.is_hostile = RandomBetween(0.0f, 1.0f) < 0.3f; // 30% chance of hostility

		// Generate description
		std::string tag_a_name = domain_tags[idx_a].name().c_str();
		std::string tag_b_name = domain_tags[idx_b].name().c_str();
		if (rule.is_hostile)
		{
			rule.description = tag_a_name + " opposes " + tag_b_name;
		}
		else
		{
			rule.description = tag_a_name + " harmonizes with " + tag_b_name;
		}
		LOG_INFO(Religion, "Religion has axiom: ", religion_entity.name().c_str(), " ", rule.description.c_str());
		axioms.rules.push_back(rule);
	}
	religion_entity.set(axioms);
}

void ReligionManager::determine_proto_stats(flecs::world& world, flecs::entity religion_entity, Cell& cell)
{
	// Generate 5 random between 0 and 1 and take the average. pseudo normal with bounding
	// honestly kind of lazy but works for now
	float proselytization = RandomBetween(0.0f, 1.0f);
	float ritual_complexity = RandomBetween(0.0f, 1.0f);
	float moral_strictness = RandomBetween(0.0f, 1.0f);
	float community_focus = RandomBetween(0.0f, 1.0f);
	float tolerance = RandomBetween(0.0f, 1.0f);
	float pacifism = RandomBetween(0.0f, 1.0f);
	float fervor = RandomBetween(0.0f, 1.0f);

	for (int i = 0; i < 5; i++)
	{
		proselytization += RandomBetween(0.0f, 1.0f);
		ritual_complexity += RandomBetween(0.0f, 1.0f);
		moral_strictness += RandomBetween(0.0f, 1.0f);
		community_focus += RandomBetween(0.0f, 1.0f);
		tolerance += RandomBetween(0.0f, 1.0f);
		pacifism += RandomBetween(0.0f, 1.0f);
		fervor += RandomBetween(0.0f, 1.0f);
	}
	ReligionStats stats;
	stats.proselytization = proselytization / 6.0f;
	stats.ritual_complexity = ritual_complexity / 6.0f;
	stats.moral_strictness = moral_strictness / 6.0f;
	stats.community_focus = community_focus / 6.0f;
	stats.tolerance = tolerance / 6.0f;
	stats.pacifism = pacifism / 6.0f;
	stats.fervor = fervor / 6.0f;
	religion_entity.set(stats);
}

void ReligionManager::generate_proto_pantheon(flecs::world& world, History& history, Cell& cell, flecs::entity religion_entity, int nr_proto_gods)
{
	auto query_archetype = world.query<Archetype>();
	std::vector<flecs::entity> archetypes;
	std::vector<float> archetype_weights;
	query_archetype.each([&](flecs::entity archetype, const Archetype& a) {
		archetypes.push_back(archetype);
		archetype_weights.push_back(1.0f);
		});
	// Generate random deities for the proto religion
	for (int i = 0; i < nr_proto_gods; i++)
	{
		// Create deity entity
		std::string deity_name = get_proto_religion_name(); // Reusing name generator for deity names
		auto deity_entity = world.entity(deity_name.c_str())
			.add<Deity>()
			.add<WorshippedBy>(religion_entity);

		// Assign random domains to deity
		auto query = world.query<Domain>();
		std::vector<flecs::entity> domains;
		query.each([&](flecs::entity domain_entity, const Domain& d) {
			domains.push_back(domain_entity);
			});
		int nr_domains = RandomBetweenInt(1, 3);
		for (int d = 0; d < nr_domains && !domains.empty(); d++)
		{
			int idx = RandomBetweenInt(0, (int)domains.size() - 1);
			deity_entity.add<GodOf>(domains[idx]);
			LOG_INFO(Religion, "Deity ", deity_name.c_str(), " assigned domain: ", domains[idx].name().c_str());
		}
		// Assign archetypes
		if (!archetypes.empty())
		{
			int archetype_index = chooseIndex(archetype_weights);
			if (archetype_index >= 0 && archetype_index < archetypes.size())
			{
				deity_entity.add<Archetype>(archetypes[archetype_index]);
				archetype_weights[archetype_index] *= 0.0f;
			}
		}

		history.log(0, EventType::DeityCreation,
			HistoricalActor(static_cast<size_t>(cell.id)),
			HistoricalActor(deity_entity),
			EventSnapshot(deity_entity.name().c_str(), "Creation", "Birth of Civilization", "The universe started.")
		);
	}
	// Proto-pantheon is filled with contradictions that are resolved in the myth phase
}

std::vector<DeityContradiction> ReligionManager::count_contradictions(flecs::world& world, flecs::entity religion_entity)
{
	std::vector<DeityContradiction> contradictions;
	// Get axioms of the religion
	const AxiomRules axioms = religion_entity.get<AxiomRules>();
	
	// 1. Collect all deities
	std::vector<flecs::entity> deities;
	world.query_builder<Deity>()
		.with<WorshippedBy>(religion_entity)
		.without<Extinct>()
		.build()
		.each([&](flecs::entity deity, const Deity& d) {
			deities.push_back(deity);
			});
	for (size_t i = 0; i < deities.size(); i++)
	{
		flecs::entity god_a = deities[i];

		// TYPE 1: Duplicate Domains (Between deities)
		for (size_t j = i + 1; j < deities.size(); j++)
		{
			flecs::entity god_b = deities[j];

			god_a.each<GodOf>([&](flecs::entity domain_a) {
				if (god_b.has<GodOf>(domain_a))
				{
					contradictions.emplace_back(religion_entity, god_a, ContradictionType::DuplicateDomain, god_b);
				}
				});
		}

		// TYPE 2: Opposing Domains (Inside the same deity)
		// Check if a deity owns two domains that are explicitly symmetric opposites
		god_a.each<GodOf>([&](flecs::entity domain_x) {
			god_a.each<GodOf>([&](flecs::entity domain_y) {
				if (domain_x != domain_y && domain_x.has<OpposedTo>(domain_y))
				{
					contradictions.emplace_back(religion_entity, god_a, ContradictionType::OpposingDomains);
				}
				});
			});

		// TYPE 3: Conflicting Axioms (Deity vs religion)
		// Check if any deity's domains violate the religion's axioms
		for (const auto& rule : axioms.rules)
		{
			if (rule.is_hostile)
			{
				// If GodOf DomainA (tag_a) and GodOf DomainB (tag_b)
				// and the religion says tag_a opposes tag_b, then contradiction
				bool has_tag_a = false;
				bool has_tag_b = false;

				god_a.each<GodOf>([&](flecs::entity domain) {
					if (domain.has<HasTag>(rule.tag_a))
						has_tag_a = true;
					if (domain.has<HasTag>(rule.tag_b))
						has_tag_b = true;
					});
				if (has_tag_a && has_tag_b)
				{
					contradictions.emplace_back(religion_entity, god_a, ContradictionType::ConflictingAxioms);
				}
			}
		}
		
	}
	return contradictions;
}

void ReligionManager::myth_phase_evolution(flecs::world& world, 
	History& history, 
	flecs::entity religion_entity, 
	uint32_t ticks_in_phase,
	vor::Voronoi& map
	)
{

	// 1. Count contradictions
	std::vector<DeityContradiction> contradictions = count_contradictions(world, religion_entity);

	if (!contradictions.empty())
	{
		// Pick a random contradiction to resolve. 
		// We only do ONE to prevent invalidating the ECS graph for the others.
		int idx = RandomBetweenInt(0, static_cast<int>(contradictions.size()) - 1);

		// Note: Update your `solve` signature to take `current_tick`
		bool solved = contradictions[idx].solve(world, history, ticks_in_phase);

		if (solved) {
			LOG_INFO(Religion, "Resolved a contradiction for ", religion_entity.name().c_str());
			// A major mythic event just happened. End the phase for this religion 
			// so the dust can settle before normal rules apply.
			return;
		}
	}

	// 2. Gather Candidates from all rules
	std::vector<std::pair<MythicRule*, RuleCandidate >> candidates;
	std::vector<float> weights;

	for (auto& rule : m_all_rules)
	{
		if (rule.cooldown_ticks > 0)
		{
			rule.cooldown_ticks--;
			continue;
		}
		if (auto cand = rule.match(world, religion_entity))
		{
			// weight calculation happens in rule
			weights.push_back(rule.get_rule_weight(world, religion_entity, map));
			candidates.push_back({ &rule, *cand });
		}
	}
	if (candidates.empty())
	{
		LOG_INFO(Religion, "No mythic rule candidates found for religion: ", religion_entity.name().c_str());
		return;
	}
	int chosen_index = chooseIndex(weights);
	auto& [selected_rule, selected_candidate] = candidates[chosen_index];
	selected_rule -> apply(world, history, selected_candidate);

	selected_rule->cooldown_ticks = selected_rule->max_ticks;


}