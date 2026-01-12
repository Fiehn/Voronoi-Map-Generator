#include "ReligionManager.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

ReligionManager::ReligionManager()
{
	// Reserve initial capacity
	m_data.reserve(256);
	m_deity_data.reserve(256);
	m_deity_relationships.reserve(256);
	
	// Initialize Religion definitions (archetypes, tenents and semantic similarity)
	InitializeReligionDefinitions();
}

void ReligionManager::InitializeReligionDefinitions()
{
	// Hardcoded, in the future maybe load from a static data file
	
	// === Archetypes ===
	
	// 1. THE SOVEREIGN
	// Primary Domain: Order, Authority, Control, Civilization
	// Examples: Zeus, Odin, Jupiter, Ra
	// Behaviour: Establish laws, maintain order, judge
	m_definitions.deity_archetypes.push_back({ "The Sovereign", 0, {} });

	// 2. THE CREATOR (DEMIURGE)
	// Domain: Creation, craftsmanship, innovation, artistry
	// Examples: Hephaestus, Ptah, Brahma, Vulcan
	// Behavior: Crafts artifacts, creates new things, transforms matter
	m_definitions.deity_archetypes.push_back({
		"The Creator",
		1,
		{}
		});

	// 3. THE SAGE
	// Domain: Wisdom, knowledge, prophecy, magic
	// Examples: Thoth, Odin (wisdom aspect), Athena, Saraswati
	// Behavior: Reveals secrets, teaches, prophesies
	m_definitions.deity_archetypes.push_back({
		"The Sage",
		2,
		{}
		});

	// 4. THE INNOCENT
	// Domain: Purity, hope, renewal, spring
	// Examples: Persephone, Horus (child), Apollo (youth aspect)
	// Behavior: Renews cycles, brings hope, represents potential
	m_definitions.deity_archetypes.push_back({
		"The Innocent",
		3,
		{}
		});

	// 5. THE EXPLORER (SEEKER)
	// Domain: Travel, discovery, boundaries, adventure
	// Examples: Hermes, Mercury, Susanoo
	// Behavior: Crosses boundaries, explores unknown, brings messages
	m_definitions.deity_archetypes.push_back({
		"The Explorer",
		4,
		{}
		});

	// 6. THE TRICKSTER
	// Domain: Chaos, change, disruption, transformation
	// Examples: Loki, Anansi, Coyote, Prometheus
	// Behavior: Challenges order, brings unexpected change, breaks rules
	m_definitions.deity_archetypes.push_back({
		"The Trickster",
		5,
		{}
		});

	// 7. THE HERO
	// Domain: Courage, strength, protection, victory
	// Examples: Ares, Thor, Sekhmet, Indra
	// Behavior: Fights threats, protects followers, seeks glory
	m_definitions.deity_archetypes.push_back({
		"The Hero",
		6,
		{}
		});

	// 8. THE LOVER
	// Domain: Love, beauty, fertility, desire, pleasure
	// Examples: Aphrodite, Freya, Ishtar, Venus
	// Behavior: Inspires passion, grants fertility, causes desire
	m_definitions.deity_archetypes.push_back({
		"The Lover",
		7,
		{}
		});

	// 9. THE CAREGIVER (GREAT MOTHER)
	// Domain: Motherhood, harvest, nourishment, protection
	// Examples: Demeter, Isis, Pachamama, Frigg
	// Behavior: Nurtures life, protects children, ensures abundance
	m_definitions.deity_archetypes.push_back({
		"The Mother",
		8,
		{}
		});

	// 10. THE DESTROYER (DEATH BRINGER)
	// Domain: Death, endings, destruction, transformation through death
	// Examples: Kali, Hades, Anubis, Morrigan
	// Behavior: Ends cycles, judges the dead, transforms through destruction
	m_definitions.deity_archetypes.push_back({
		"The Destroyer",
		9,
		{}
		});

	// 11. THE MAGICIAN
	// Domain: Transformation, alchemy, illusion, hidden knowledge
	// Examples: Hecate, Circe, Thoth (magic aspect)
	// Behavior: Transforms reality, works with hidden forces, shapeshifts
	m_definitions.deity_archetypes.push_back({
		"The Magician",
		10,
		{}
		});

	// 12. THE EVERYMAN
	// Domain: Mediation, balance, justice, common good
	// Examples: Osiris, Maat, Tyr (justice aspect)
	// Behavior: Mediates conflicts, ensures fairness, represents mortals
	m_definitions.deity_archetypes.push_back({
		"The Intercessor",
		11,
		{}
		});

	// 13. THE SHADOW / ADVERSARY
	// Domain: Evil, temptation, opposition, necessary darkness
	// Examples: Set, Ahriman, aspects of Loki
	// Behavior: Opposes order, tests mortals, represents necessary evil
	m_definitions.deity_archetypes.push_back({
		"The Shadow",
		12,
		{}
		});

	// 14. THE GUIDE 
	// Domain: Transitions, underworld guidance, liminal spaces
	// Examples: Hermes (guide of souls), Anubis, Charon
	// Behavior: Guides souls, guards thresholds, oversees transitions
	m_definitions.deity_archetypes.push_back({
		"The Psychopomp",
		13,
		{}
		});

	// 15. THE WILD ONE / NATURE SPIRIT
	// Domain: Wilderness, animals, untamed nature, instinct
	// Examples: Pan, Artemis, Cernunnos, Diana
	// Behavior: Protects wilderness, embodies natural forces, resists civilization
	m_definitions.deity_archetypes.push_back({
		"The Wild One",
		14,
		{}
		});

	// 16. THE JESTER / FOOL
	// Domain: Humor, absurdity, wisdom through folly, subversion
	// Examples: Bes, aspects of Dionysus
	// Behavior: Brings joy, reveals truth through humor, breaks tension
	m_definitions.deity_archetypes.push_back({
		"The Jester",
		15,
		{}
		});

	// === Tenets ===
	// TODO: Find out how to do tenets well

	// === Fill out enum storages === 
	m_definitions.all_domains = { DeityDomain::War,
	DeityDomain::Fertility,
	DeityDomain::Wisdom,
	DeityDomain::Love,
	DeityDomain::Nature,
	DeityDomain::Sea,
	DeityDomain::Sky,
	DeityDomain::Forests,
	DeityDomain::Underworld,
	DeityDomain::Horses,
	DeityDomain::Metals,
	DeityDomain::Storms,
	DeityDomain::Music,
	DeityDomain::Trade,
	DeityDomain::Wind,
	DeityDomain::Fire,
	DeityDomain::Death,
	DeityDomain::Darkness,
	DeityDomain::Sun,
	DeityDomain::Travel,
	DeityDomain::Moon,
	DeityDomain::Order,
	DeityDomain::Chaos,
	DeityDomain::Mountains,
	DeityDomain::Winter,
	DeityDomain::Water,
	DeityDomain::Law};

	m_definitions.all_deity_relationships = { DeityRelationship::ParentOf,
	DeityRelationship::SpouseOf,
	DeityRelationship::ServantOf,
	DeityRelationship::EnemyOf,
	DeityRelationship::SiblingOf,
	DeityRelationship::SlayerOf,
	DeityRelationship::ProtectorOf,
	DeityRelationship::CreatorOf,
	DeityRelationship::AuthorOf,
	DeityRelationship::MutilatorOf };

	m_definitions.all_religion_types = {
		ReligionType::Animistic,
		ReligionType::Philosophical,
		ReligionType::Mystical,
		ReligionType::Monotheistic,
		ReligionType::Polytheistic,
		ReligionType::Humanistic,
		ReligionType::Atheistic
	};

	// === Semantic Similarity Table Load ===
	m_definitions.domain_semantic_similarity = LoadSemanticSimilarityTable(m_definitions.all_domains);

}

std::vector<std::vector<float>> ReligionManager::LoadSemanticSimilarityTable(const std::vector<DeityDomain>& all_domains)
{
	// In assets folder a CSV file defines the semantic similarity between deity domains
	// Load the CSV file and parse its contents

	std::vector<std::vector<float>> table;

	const std::string filename = "Assets/SemanticSimilarity/DomainKeywordSimilarities.csv";
	
	std::ifstream file(filename);
	if (!file.is_open()) {
		LOG_WARNING(Culture, "Could not open semantic similarity file", filename.c_str());
		// Return default table with zeros
		table.resize(all_domains.size(), std::vector<float>(all_domains.size(), 0.0f));
		return table;
	}

	// Read CSV file
	std::string line;
	bool firstLine = true;
	
	while (std::getline(file, line)) {
		// Skip header row if present
		if (firstLine) {
			firstLine = false;
			// You could parse header here if needed
			continue;
		}

		std::vector<float> row;
		std::stringstream ss(line);
		std::string value;

		// Parse each value in the row
		while (std::getline(ss, value, ',')) {
			try {
				float similarity = std::stof(value);
				row.push_back(similarity);
			}
			catch (const std::exception&) {
				// If conversion fails, use 0.0
				row.push_back(0.0f);
			}
		}

		if (!row.empty()) {
			table.push_back(row);
		}
	}

	file.close();

	// Validate table dimensions
	if (table.size() != all_domains.size()) {
		LOG_WARNING(Culture, "Semantic similarity table size mismatch", table.size(), all_domains.size());
		// Resize to match
		table.resize(all_domains.size(), std::vector<float>(all_domains.size(), 0.0f));
	}

	// Ensure all rows have correct size
	for (auto& row : table) {
		if (row.size() != all_domains.size()) {
		 row.resize(all_domains.size(), 0.0f);
		}
	}

	return table;
}

ReligionHandle ReligionManager::CreateReligion(const ReligionCreateInfo& info)
{
	// Allocate new slot
	uint16_t index = AllocateSlot();
	// If we grew, resize all data arrays
	if (index >= m_data.size()) {
		m_data.resize(index + 1);
	}
	
	// Initialize generation FIRST (required for IsValid to work)
	m_data.generations[index] = m_next_generation;
	
	// Initialize Religion data
	m_data.names[index] = info.name;
	m_data.religion_types[index] = info.religion_type;
	m_data.parent_religion_id[index] = 0; // Proto-religion
	m_data.founding_dates[index] = info.founding_date;
	m_data.origin_cell_ids[index] = info.origin_cell_id;
	m_data.divergence_rates[index] = 0.0f; // No divergence for proto-religion
	// Initialize religious traits
	m_data.proselytization_rate[index] = info.proselytization_rate;
	m_data.ritual_complexity[index] = info.ritual_complexity;
	m_data.afterlife_belief_strength[index] = info.afterlife_belief_strength;
	m_data.moral_strictness[index] = info.moral_strictness;
	m_data.clergy_influence[index] = info.clergy_influence;
	m_data.community_focus[index] = info.community_focus;
	m_data.tolerance_level[index] = info.tolerance_level;
	m_data.pacifism_level[index] = info.pacifism_level;
	// Initialize tenents, holy cities, and pantheon
	m_data.tenents[index] = info.tenents;
	m_data.holy_city_cell_ids[index] = info.holy_city_cell_ids;
	
	m_data.pantheons[index].clear(); // Ensure it's properly initialized
	m_data.pantheons[index].reserve(16); // Reserve some space for deities
	
	// Create handle
	ReligionHandle handle;
	handle.index = index;
	handle.generation = m_next_generation++;
	
	return handle;
}

ReligionHandle ReligionManager::DeriveReligion(ReligionHandle parent_handle, const ReligionCreateInfo& info)
{
	// Validate parent handle
	if (!IsValid(parent_handle)) {
		throw std::runtime_error("Invalid parent religion handle in DeriveReligion.");
	}
	uint16_t parent_index = parent_handle.index;
	// Allocate new slot
	uint16_t index = AllocateSlot();
	// If we grew, resize all data arrays
	if (index >= m_data.size()) {
		m_data.resize(index + 1);
	}
	// Initialize religion data
	m_data.names[index] = info.name;
	m_data.religion_types[index] = info.religion_type;
	m_data.parent_religion_id[index] = parent_index;
	m_data.founding_dates[index] = info.founding_date;
	m_data.origin_cell_ids[index] = info.origin_cell_id;
	m_data.divergence_rates[index] = 0.0f; // Start with no divergence
	// Inherit traits from parent religion
	// (Placeholder logic, real implementation would blend traits)
	m_data.proselytization_rate[index] = info.proselytization_rate;
	m_data.ritual_complexity[index] = info.ritual_complexity;
	m_data.afterlife_belief_strength[index] = info.afterlife_belief_strength;
	m_data.moral_strictness[index] = info.moral_strictness;
	m_data.clergy_influence[index] = info.clergy_influence;
	m_data.community_focus[index] = info.community_focus;
	m_data.tolerance_level[index] = info.tolerance_level;
	m_data.pacifism_level[index] = info.pacifism_level;
	// Initialize tenents, holy cities, and pantheon
	m_data.tenents[index] = info.tenents;
	m_data.holy_city_cell_ids[index] = info.holy_city_cell_ids;
	
	m_data.pantheons[index].clear(); // Ensure it's properly initialized
	m_data.pantheons[index].reserve(16); // Reserve some space for deities
	
	// Create handle
	ReligionHandle handle;
	handle.index = index;
	handle.generation = m_next_generation++;
	return handle;
}

void ReligionManager::MarkExtinct(ReligionHandle handle)
{
	if (!IsValid(handle)) {
		return; // Invalid handle
	}
	uint16_t index = handle.index;
	// TODO: Fix extingtion logic like in language
}

bool ReligionManager::IsValid(ReligionHandle handle) const
{
	if (handle.index >= m_data.size()) {
		LOG_WARNING(Culture, "Religion is invalid. index is larger than size.", handle.index, m_data.size());
		return false;
	}
	if (handle.generation == 0) {
		LOG_WARNING(Culture, "Religion is invalid. generation is 0.", handle.index, handle.generation);
		return false;
	}
	if (m_data.generations[handle.index] != handle.generation) {
		LOG_WARNING(Culture, "Religion is invalid. generation mismatch.", handle.index, handle.generation, m_data.generations[handle.index]);
		return false;
	}
	return true;
}

uint16_t ReligionManager::AllocateSlot()
{
	// This function needs to return the next available index and grow the data arrays if needed
	uint16_t index = static_cast<uint16_t>(m_data.size());

	m_data.resize(index + 1);

	return index;
}

uint16_t ReligionManager::AllocateDeitySlot()
{
	// Allocate a new deity slot
	uint16_t index = static_cast<uint16_t>(m_deity_data.size());
	
	// Resize all deity data arrays
	m_deity_data.resize(index + 1);
	
	// Initialize the new slot's relationship data (use index assignment, not push_back)
	m_deity_data.relationship_offsets[index] = static_cast<uint32_t>(m_deity_relationships.source_deity_ids.size());
	m_deity_data.relationship_counts[index] = 0;
	m_deity_data.generations[index] = m_next_deity_generation;
	
	return index;
}

std::string ReligionManager::GetReligionName(ReligionHandle handle) const
{
	if (!IsValid(handle)) {
		return "Invalid Religion";
	}
	return m_data.names[handle.index];
}

std::string ReligionManager::GetDeityName(DeityHandle handle) const
{
	if (!handle.isValid() || handle.index >= m_deity_data.names.size()) {
		return "Invalid Deity";
	}
	return m_deity_data.names[handle.index];
}

std::vector<size_t> ReligionManager::GetHolyCities(ReligionHandle handle) const
{
	if (!IsValid(handle)) {
		return {};
	}
	return m_data.holy_city_cell_ids[handle.index];
}

void ReligionManager::UpdateReligions(vor::Voronoi& voronoi, uint32_t current_tick)
{
	// Placeholder for religion update logic
	// This would include spreading, schisms, reforms, etc.
	// TODO: Implement when simulations are ready
}

void ReligionManager::GenerateProtoPantheon(ReligionHandle handle, Cell& origin_cell)
{ // Generates a basic pantheon for a new proto-religion based on the origin cell properties
	// Algorithm:
	// 1. If relevant generate a pantheon
	//		a. Constrained Satisfaction Problem:
	//		b. Assign amount of Deities
	//		c. Calculate probabilities for Archetypes
	//		d. Assign Archetypes
	//		e. Calculate probabilities for domains based on cell geography, archetypes
	//		f. While conflict count > 0.5 * deity count:
	//			1. Pick a Deity: Detect conflicts in archetypes and domains
	//			2. Repair: change domain or archetype so as to minimize the total nr of conflicts
	// 2. Key Constraints and factors:
	//		a. Exclusivity: no two gods can have the same primary domain
	//		b. Thematic Adjacency: Semantic similarity between all domains cannot be too large
	//		c. Thematic Opposition: A deity cannot hold chaos and order at the same time
	//		d. Environment factor: River god will always exist near rivers but won't exist in mountains (and so on)
	//		e. Archetypes factor: Archetypes have natural domains that increase probability

	int amount = 0;
	if (m_data.religion_types[handle.index] == ReligionType::Monotheistic)
		amount = 1;
	else if (m_data.religion_types[handle.index] == ReligionType::Polytheistic || m_data.religion_types[handle.index] == ReligionType::Mystical)
	{
		amount = RandomBetweenInt(3, 16);
	}
;
	std::vector<float> archetype_weights(m_definitions.deity_archetypes.size(), 1.0f);
	// The first god is often the sovereign, creator, or mother archetype
	archetype_weights[0] += 2.0f; // Sovereign
	archetype_weights[1] += 1.5f; // Creator
	archetype_weights[8] += 1.5f; // Mother

	// === Generate Deities ===
	for (int god = 0; god < amount; god++)
	{
		uint16_t deity_index = AllocateDeitySlot();
		DeityHandle deity_handle;
		deity_handle.index = deity_index;
		deity_handle.generation = m_next_deity_generation++;
		
		// Assign archetype
		uint16_t archetype_id = static_cast<uint16_t>(chooseIndex(archetype_weights));

		// Update archetype weights to reduce chance of repeats
		if (god == 0)
		{
			// Reset all weights first (if the first god was not one of the boosted then it goes back to not being boosted)
			std::fill(archetype_weights.begin(), archetype_weights.end(), 1.0f);

			// First god gets a bigger reduction to encourage variety
			archetype_weights[archetype_id] *= 0.01f;
		}
		else
		{
			archetype_weights[archetype_id] *= 0.2f;
		}

		// Store deity data using index (not push_back)
		m_deity_data.archetype_ids[deity_index] = archetype_id;
		m_deity_data.source_religion_ids[deity_index] = handle.index;
		m_deity_data.prominence_levels[deity_index] = 1.0f; // Initialize with default value
		m_deity_data.benevolence_levels[deity_index] = 0.5f; // Initialize with default value
		
		// Generate name
		GenerateDeityName(deity_handle, handle);
		// Generate domains
		GenerateDeityDomains(deity_handle, origin_cell);

		// Add to pantheon
		AddDeityToPantheon(handle, deity_handle);
	}

	// === Amend Conflicts ===
	int conflict_count = CountDeityConflicts(handle, origin_cell);
	int iter = 0;
	LOG_INFO(Culture, "Initial deity conflict count for deity: ", conflict_count);

	while (conflict_count > 0)
	{
		int deity_to_repair = RandomBetweenInt(0, amount - 1);
		RepairDeityConflict(deity_to_repair);
		LOG_INFO(Culture, "Repairing deity conflict in pantheon generation.", deity_to_repair, conflict_count);
		conflict_count = CountDeityConflicts(handle, origin_cell);
		LOG_INFO(Culture, "Post-repair conflict count.", conflict_count);
		iter++;
		if (iter > 1000) {
			LOG_WARNING(Culture, "Pantheon generation exceeded max iterations.");
			break;
		}
	}

	// === Generate Relationships ===
	std::vector<DeityHandle> pantheon_deities = GetPantheonDeities(handle);
	for (const DeityHandle& deity : pantheon_deities)
	{
		GenerateDeityRelationships(deity, pantheon_deities);
	}
		
}

int ReligionManager::CountDeityConflicts(ReligionHandle religion_handle, const Cell& origin_cell) const
{
	// Conflicts are defined as:
	// 1. Two deities sharing the same primary domain
	// 2. Two deities having high semantic similarity in their domains
	// 3. A deity holding opposing domains (e.g., Order and Chaos)
	// 4. A deity having domains that contradict the environment of the origin cell
	if (!IsValid(religion_handle)) {
		LOG_WARNING(Culture, "CountDeityConflicts called with invalid religion handle.");
		return 0;
	}
	const Pantheon& pantheon = GetPantheon(religion_handle);
	int conflict_count = 0;

	// Get origin cell for environment checks
	const size_t origin_cell_id = m_data.origin_cell_ids[religion_handle.index];

	// === Check for conflicts between deity pairs ===
	for (size_t i = 0; i < pantheon.deities.size(); i++)
	{
		const DeityHandle& deity_a = pantheon.deities[i];
		const std::vector<DeityDomain>& domains_a = m_deity_data.domains[deity_a.index];

		// === 1. Check for self-Contradictions ===
		bool has_order = std::find(domains_a.begin(), domains_a.end(), DeityDomain::Order) != domains_a.end();
		bool has_chaos = std::find(domains_a.begin(), domains_a.end(), DeityDomain::Chaos) != domains_a.end();
		
		if (has_order && has_chaos) {
			conflict_count++; // TODO: add other contradictory domain pairs
		}

		// === 2. Check Conflicts with other deities ===
		for (size_t j = i + 1; j < pantheon.deities.size(); j++)
		{
			const DeityHandle& deity_b = pantheon.deities[j];
			const std::vector<DeityDomain>& domains_b = m_deity_data.domains[deity_b.index];
			// a. Check for shared primary domain
			if (!domains_a.empty() && !domains_b.empty() && domains_a[0] == domains_b[0]) {
				conflict_count++;
			}
			// b. Check for high semantic similarity
			float similarity = SemanticInterceptionBetweenSects(domains_a, domains_b);
			const float similarity_threshold = 0.8f; // Arbitrary threshold
			if (similarity > similarity_threshold) {
				conflict_count++;
			}
		}
	}
	// === 3. Check for environment contradictions ===
	// TODO: Implement environment-based conflict checks based on origin_cell properties
	

	return conflict_count;
}

float ReligionManager::SemanticInterceptionBetweenSects(const std::vector<DeityDomain>& domains_a, const std::vector<DeityDomain>& domains_b) const
{
	// Use semantic table to calculate average similarity between two sets of domains
	float total_similarity = 0.0f;
	int comparisons = 0;
	for (const DeityDomain& domain_a : domains_a)
	{
		for (const DeityDomain& domain_b : domains_b)
		{
			size_t index_a = static_cast<size_t>(domain_a);
			size_t index_b = static_cast<size_t>(domain_b);
			if (index_a < m_definitions.domain_semantic_similarity.size() &&
				index_b < m_definitions.domain_semantic_similarity[index_a].size())
			{
				total_similarity += m_definitions.domain_semantic_similarity[index_a][index_b];
				comparisons++;
			}
		}
	}
	if (comparisons == 0) {
		return 0.0f;
	}
	return total_similarity / static_cast<float>(comparisons);
}

void ReligionManager::RepairDeityConflict(int deity_index)
{
	// Identify Conflict and fix by:
	// 1. Changing a domain to reduce conflicts
	// 2. Changing archetype to reduce conflicts
	// 3. Remove redundant Domains

	if (deity_index >= m_deity_data.size()) 
	{
		LOG_WARNING(Culture, "RepairDeityConflict called with invalid deity index.", deity_index);
		return;
	}

	std::vector<DeityDomain>& domains = m_deity_data.domains[deity_index];

	// === Fix self-contradictions ===
	bool has_order = std::find(domains.begin(), domains.end(), DeityDomain::Order) != domains.end();
	bool has_chaos = std::find(domains.begin(), domains.end(), DeityDomain::Chaos) != domains.end();

	if (has_order && has_chaos) {
		// Remove one of the contradictory domains
		if (RandomBetween(0.0f, 1.0f) < 0.5f) {
			domains.erase(std::remove(domains.begin(), domains.end(), DeityDomain::Order), domains.end());
		} else {
			domains.erase(std::remove(domains.begin(), domains.end(), DeityDomain::Chaos), domains.end());
		}
	}

	// === Determine domains that cause conflicts with other deities ===
	for (size_t other_deity_index = 0; other_deity_index < m_deity_data.size(); other_deity_index++)
	{
		if (other_deity_index == static_cast<size_t>(deity_index)) {
			continue; // Skip self
		}
		const std::vector<DeityDomain>& other_domains = m_deity_data.domains[other_deity_index];
		// a. Check for shared domain
		std::vector<int> domain_idx_to_change;
		for (size_t d = 0; d < domains.size(); d++)
		{
			if (std::find(other_domains.begin(), other_domains.end(), domains[d]) != other_domains.end()) {
				domain_idx_to_change.push_back(static_cast<int>(d));
			}
		}
		if (!domain_idx_to_change.empty()) {
			// Change the conflicting domain
			for (int idx : domain_idx_to_change) {
				domains[idx] = GetRandomDomainExcluding(domains);
			}
		}
		// b. Check for high semantic similarity
		float similarity = SemanticInterceptionBetweenSects(domains, other_domains);
		const float similarity_threshold = 0.8f; // Arbitrary threshold
		if (similarity > similarity_threshold) {
			// Change a domain to reduce similarity
			// Only attempt to change a random domain if we have more than one domain
			if (domains.size() > 1) {
				domains[RandomBetweenInt(0, static_cast<int>(domains.size()) - 1)] = GetRandomDomainExcluding(domains);
			} else if (domains.size() == 1) {
				// If only one domain, replace it directly
				domains[0] = GetRandomDomainExcluding(domains);
			}
		}
	}

}
DeityDomain ReligionManager::GetRandomDomainExcluding(const std::vector<DeityDomain>& exclude_domains) const
{
	std::vector<DeityDomain> possible_domains;
	for (const DeityDomain& domain : m_definitions.all_domains)
	{
		if (std::find(exclude_domains.begin(), exclude_domains.end(), domain) == exclude_domains.end()) {
			possible_domains.push_back(domain);
		}
	}
	if (possible_domains.empty()) {
		return DeityDomain::War; // Fallback
	}
	size_t index = static_cast<size_t>(RandomBetweenInt(0, static_cast<int>(possible_domains.size()) - 1));
	return possible_domains[index];
}

void ReligionManager::DerivePantheon(ReligionHandle child_handle, ReligionHandle parent_handle, Cell& origin_cell_child)
{
	// Derives a pantheon for a new religion based on the parent religion's pantheon and the origin cell properties
	// Will add and remove some deities, and modify relationships based on the new environment
	// Also some dieties may be dropped or merged
}
void ReligionManager::Syncretism(ReligionHandle religon_a, ReligionHandle religion_b, Cell& contact_cell, float relative_strength)
{
	// Merges elements of two religions when they come into contact in a cell
	// The relative_strength parameter determines which religion has more influence in the syncretism process
	// Will merge pantheons, blend tenents, and adjust traits accordingly
	// old gods will be cleaned up after merging by removing, killing off, or merging deities
}
void ReligionManager::AddDeityRelationship(DeityHandle source, DeityHandle target, DeityRelationship relationship_type, float strength)
{
	// Adds a relationship between two deities
	if (!source.isValid() || !target.isValid()) {
		LOG_WARNING(Culture, "AddDeityRelationship Called with invalid handles.");
		return;
	}

	// Get current offset and count for source deity
	uint32_t offset = m_deity_data.relationship_offsets[source.index];
	uint32_t count = m_deity_data.relationship_counts[source.index];

	// Insert at the end of this deity's relationship range
	uint32_t insert_pos = offset + count;

	// Add to the shared relationship pool
	m_deity_relationships.source_deity_ids.insert(
		m_deity_relationships.source_deity_ids.begin() + insert_pos, source.index);
	m_deity_relationships.target_deity_ids.insert(
		m_deity_relationships.target_deity_ids.begin() + insert_pos, target.index);
	m_deity_relationships.relationship_types.insert(
		m_deity_relationships.relationship_types.begin() + insert_pos, relationship_type);
	m_deity_relationships.relationship_strengths.insert(
		m_deity_relationships.relationship_strengths.begin() + insert_pos, strength);

	// Update count for source
	m_deity_data.relationship_counts[source.index]++;

	// Update offsets for all deities after this one
	for (size_t i = source.index + 1; i < m_deity_data.size(); i++) {
		m_deity_data.relationship_offsets[i]++;
	}
}

void ReligionManager::RemoveDeityRelationship(DeityHandle source, DeityHandle target, DeityRelationship relationship_type)
{
	// Removes a relationship between two deities
	if (!source.isValid() || !target.isValid()) {
		LOG_WARNING(Culture, "RemoveDeityRelationship called with invalid handles.");
		return;
	}

	uint32_t offset = m_deity_data.relationship_offsets[source.index];
	uint32_t count = m_deity_data.relationship_counts[source.index];

	// Find and remove the relationship
	for (uint32_t i = 0; i < count; i++) {
		uint32_t idx = offset + i;
		if (m_deity_relationships.target_deity_ids[idx] == target.index && 
			m_deity_relationships.relationship_types[idx] == relationship_type) {
			
			// Remove this relationship from all arrays
			m_deity_relationships.source_deity_ids.erase(
				m_deity_relationships.source_deity_ids.begin() + idx);
			m_deity_relationships.target_deity_ids.erase(
				m_deity_relationships.target_deity_ids.begin() + idx);
			m_deity_relationships.relationship_types.erase(
				m_deity_relationships.relationship_types.begin() + idx);
			m_deity_relationships.relationship_strengths.erase(
				m_deity_relationships.relationship_strengths.begin() + idx);

			// Update count
			m_deity_data.relationship_counts[source.index]--;

			// Update offsets for all subsequent deities
			for (size_t j = source.index + 1; j < m_deity_data.size(); j++) {
				m_deity_data.relationship_offsets[j]--;
			}
			return;
		}
	}
}

std::vector<DeityHandle> ReligionManager::GetRelatedDeities(DeityHandle deity, DeityRelationship relationship_type) const
{
	std::vector<DeityHandle> result;

	if (!deity.isValid() || deity.index >= m_deity_data.size()) {
		LOG_WARNING(Culture, "GetRelatedDeities called with invalid handles.");
		return result;
	}

	uint32_t offset = m_deity_data.relationship_offsets[deity.index];
	uint32_t count = m_deity_data.relationship_counts[deity.index];

	for (uint32_t i = 0; i < count; i++) {
		uint32_t idx = offset + i;
		if (m_deity_relationships.relationship_types[idx] == relationship_type) {
			DeityHandle target;
			target.index = m_deity_relationships.target_deity_ids[idx];
			target.generation = m_deity_data.generations[target.index];
			result.push_back(target);
		}
	}

	return result;
}
void ReligionManager::AddDeityToPantheon(ReligionHandle religion_handle, DeityHandle deity_handle)
{
	if (!IsValid(religion_handle) || !deity_handle.isValid()) {
		LOG_WARNING(Culture, "AddDeityToPantheon called with invalid handles.");
		return;
	}

	Pantheon& pantheon = m_data.pantheons[religion_handle.index];

	// Check if deity is already in pantheon
	if (std::find(pantheon.deities.begin(), pantheon.deities.end(), deity_handle) != pantheon.deities.end()) {
		LOG_INFO(Culture, "Deity added to a pantheon multiple times!");
		return; // Already in pantheon
	}

	pantheon.deities.push_back(deity_handle);
}
void ReligionManager::RemoveDeityFromPantheon(ReligionHandle religion_handle, DeityHandle deity_handle)
{
	if (!IsValid(religion_handle) || !deity_handle.isValid()) {
		LOG_WARNING(Culture, "RemoveDeityFromPantheon called with invalid handles.");
		return;
	}

	Pantheon& pantheon = m_data.pantheons[religion_handle.index];
	pantheon.deities.erase(
		std::remove(pantheon.deities.begin(), pantheon.deities.end(), deity_handle),
		pantheon.deities.end()
	);
}
const Pantheon& ReligionManager::GetPantheon(ReligionHandle religion_handle) const
{
	// Retrieves the pantheon of the specified religion
	static Pantheon empty_pantheon;
	if (!IsValid(religion_handle)) {
		return empty_pantheon;
	}
	return m_data.pantheons[religion_handle.index];
}
std::vector<DeityHandle> ReligionManager::GetPantheonDeities(ReligionHandle religion_handle) const
{
	// Retrieves the list of deities in the pantheon of the specified religion
	if (!IsValid(religion_handle)) {
		return {};
	}
	return m_data.pantheons[religion_handle.index].deities;
}
void ReligionManager::GenerateDeityName(DeityHandle deity, ReligionHandle religion_handle)
{
	// Generates a name for the deity based on the religion and archetype
	// TODO: Implement name generation logic with language integration
	std::vector<std::string> possible_syllables = { "an", "or", "el", "ra", "zu", "ma", "ti", "ka", "lo", "vi" };

	int syllable_count = RandomBetweenInt(2, 4);
	std::string name;
	for (int i = 0; i < syllable_count; i++) {
		size_t index = static_cast<size_t>(RandomBetweenInt(0, static_cast<int>(possible_syllables.size()) - 1));
		name += possible_syllables[index];
	}
	m_deity_data.names[deity.index] = name;
}

void ReligionManager::GenerateDeityDomains(DeityHandle deity, const Cell& origin_cell)
{
	// Generates domains for the deity based on the origin cell properties
	// The goal is to assign domains that fit the environment and archetype of the deity
	// 1-3 domains per deity
	std::vector<DeityDomain> assigned_domains;
	int domain_count = RandomBetweenInt(1, 3);
	
	std::vector<float> domain_weights(m_definitions.all_domains.size(), 1.0f);
	// Adjust weights based on origin cell properties
	if (origin_cell.riverBool) {
		domain_weights[static_cast<size_t>(DeityDomain::Trade)] += 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Fertility)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Nature)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Travel)] += 1.0f;
		// TODO: Update when more domains are added
	}
	if (origin_cell.coastBool || origin_cell.distToOcean < 4)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Sea)] += 3.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Storms)] += 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Wind)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Travel)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Fertility)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Chaos)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Sky)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Metals)] -= 0.9f;
		// TODO: Update when more domains are added
	}
	if (origin_cell.height > 0.9)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Mountains)] += 3.0f;
		domain_weights[static_cast<size_t>(DeityDomain::War)] += 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Metals)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Sky)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Order)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Chaos)] -= 0.9f;
	}
	if (origin_cell.tempVariance > 5)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Storms)] += 2.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Chaos)] += 1.5f;
	}
	if (origin_cell.temp > 35)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Sun)] += 2.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Fire)] += 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Fertility)] += 1.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Winter)] -= 1.0f;
	}
	if (origin_cell.temp < 0)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Winter)] += 2.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Death)] += 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Chaos)] += 1.0f;
	}
	if (origin_cell.volcanicActivity == true)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Fire)] += 3.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Underworld)] += 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Chaos)] += 1.0f;
	}
	if (origin_cell.windStr >= 0.9)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Wind)] += 2.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Storms)] += 1.5f;
	}
	if (origin_cell.percepitation < 1 || origin_cell.humidity < 0.2)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Sun)] += 2.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Water)] += 1.5f;
		}
	if (origin_cell.distToOcean > 10 && !origin_cell.riverBool)
	{
		domain_weights[static_cast<size_t>(DeityDomain::Sea)] -= 3.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Water)] -= 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Horses)] += 1.0f;
	}
	// TODO: WAAY more things please
	
	// Archetype based adjustments to weights
	if (m_deity_data.archetype_ids[deity.index] == 0) // Sovereign
	{
		domain_weights[static_cast<size_t>(DeityDomain::Order)] += 2.0f;
		domain_weights[static_cast<size_t>(DeityDomain::Law)] += 1.5f;
		domain_weights[static_cast<size_t>(DeityDomain::Chaos)] -= 1.0f;
}
	// TODO: Add more archetype based adjustments


	// Make sure that all weights are non-negative
	for (float& weight : domain_weights) {
		if (weight < 0.0f) {
			weight = 0.0f;
		}
	}

	// Assign domains
	for (int i = 0; i < domain_count; i++)
	{
		DeityDomain domain = static_cast<DeityDomain>(chooseIndex(domain_weights));
		assigned_domains.push_back(domain);
		// Delete weight to avoid duplicates 
		domain_weights[static_cast<size_t>(domain)] *= 0.0f;
	}

	m_deity_data.domains[deity.index] = assigned_domains;
}
void ReligionManager::GenerateDeityRelationships(DeityHandle deity, const std::vector<DeityHandle>& pantheon)
{
	// Utilizing Graph Grammars (Graph Rewriting Systems) to generate relationships between deities in the pantheon
	
	// Rules:
	// 1. Spousality
	//    a. Input: Node(a) + Node(b) + Edge(None) + probability threshold t + domain similarity s
	//    b. Output: Node(a) + Node(b) + Edge(Spouse)
	// 2. Generational Biring
	//    a. Input: Node(a) + Node(b) + Edge(Spouse)
	//    b. Output: Node(child) + edge(parent, a, child) + edge(parent, b, child)
	//    c. Logic: Child's domain is SemanticIntersection(domains of a and b)
	// 3. Rivalry Emergence
	//    a. Input: Node(a) + Node(b) + Edge(Sibling)
	//    b. Output: Edge(Enemy, a, b) with probability p based on domain similarity
	// 4. Protector Assignment
	//    a. Input: Node(a) + Node(b)
	//    b. Output: Edge(Protector, a, b) if a's domains complement b's vulnerabilities
	// 5. Creator-Servant Relationship
	//    a. Input: Node(creator)
	//    b. Output: Node(servant) + Edge(CreatorOf, creator, servant) + Edge(ServantOf, servant, creator)
	//   c. Logic: Servant's domain is derived from creator's lesser domains
	// 6. Mutilation Myth
	//    a. Input: Node(a)
	//    b. Output: Edge(MutilatorOf, a, b) + Node(broken_aspect)
	//   c. Logic: broken_aspect's domain is a distorted version of a's primary domain
	// 7. Balance of Power
	//    a. Input: Node(a) + Node(b)
	//    b. Output: Edge(Enemy, a, b) if domains are highly similar
	
	if (pantheon.empty() || !deity.isValid()) {
		LOG_WARNING(Culture, "GenerateDeityRelationships called with invalid parameters.");
		return;
	}
		
	// Get deity data for refrence
	const std::vector<DeityDomain>& deity_domains = m_deity_data.domains[deity.index];
	const uint16_t deity_archetype = m_deity_data.archetype_ids[deity.index];
		
	// === Rule 1: Spousality ===
	for (const DeityHandle& other_deity : pantheon)
	{
		// Skip self
		if (other_deity.index == deity.index) {
			continue;
		}
		// Check if already spouses
		std::vector<DeityHandle> spouses = GetRelatedDeities(deity, DeityRelationship::SpouseOf);
		if (std::find(spouses.begin(), spouses.end(), other_deity) != spouses.end()) {
			continue; // Already spouses
		}
		// Calculate domain similarity
		const std::vector<DeityDomain>& other_domains = m_deity_data.domains[other_deity.index];
		float similarity = SemanticInterceptionBetweenSects(deity_domains, other_domains);
		// Probability threshold for spousality
		float spousality_probability = similarity * 0.5f; // Scale factor
		if (RandomBetween(0.0f, 1.0f) < spousality_probability)
		{
			AddDeityRelationship(deity, other_deity, DeityRelationship::SpouseOf, spousality_probability);
			AddDeityRelationship(other_deity, deity, DeityRelationship::SpouseOf, spousality_probability);
		}
	}

	// === Rule 2: Generational Biring ===
	for (const DeityHandle& other_deity : pantheon)
	{
		if (other_deity.index == deity.index) {
			continue; // Skip self
		}

		// Check for spouse relationship
		std::vector<DeityHandle> spouses = GetRelatedDeities(deity, DeityRelationship::SpouseOf);
		if (std::find(spouses.begin(), spouses.end(), other_deity) != spouses.end())
		{
			for (const DeityHandle& potential_child : pantheon)
			{
				if (potential_child.index == deity.index || potential_child.index == other_deity.index) {
					continue; // Skip self and spouse
				}
				// Check if childs domains are intersection of parents
				const std::vector<DeityDomain>& child_domains = m_deity_data.domains[potential_child.index];
				const std::vector<DeityDomain>& other_domains = m_deity_data.domains[other_deity.index];

				float parent_similarity = (SemanticInterceptionBetweenSects(deity_domains, child_domains) +
					SemanticInterceptionBetweenSects(other_domains, child_domains)) / 2.0f;

				if (parent_similarity > 0.4f) // Threshold for parent-child relationship
				{
					AddDeityRelationship(deity, potential_child, DeityRelationship::ParentOf, parent_similarity);
					AddDeityRelationship(other_deity, potential_child, DeityRelationship::ParentOf, parent_similarity);
				}
			}
		}
		// If no spouse, lets create bastards
		for (const DeityHandle& potential_child : pantheon)
		{
			if (potential_child.index == deity.index) {
				continue; // Skip self
			}
			// Check if childs domains are similar to single parent
			const std::vector<DeityDomain>& child_domains = m_deity_data.domains[potential_child.index];
			float parent_similarity = SemanticInterceptionBetweenSects(deity_domains, child_domains);
			if (parent_similarity > 0.6f) // Higher threshold for single parent
			{
				AddDeityRelationship(deity, potential_child, DeityRelationship::ParentOf, parent_similarity);
			}
		}
	}
	// === Rule 2.5: Sibling Relationship ===
	for (const DeityHandle& other_deity : pantheon)
	{
		if (other_deity.index == deity.index) {
			continue; // Skip self
		}
		// Check for shared parents
		std::vector<DeityHandle> parents_a = GetRelatedDeities(deity, DeityRelationship::ParentOf);
		std::vector<DeityHandle> parents_b = GetRelatedDeities(other_deity, DeityRelationship::ParentOf);
		for (const DeityHandle& parent_a : parents_a)
		{
			if (std::find(parents_b.begin(), parents_b.end(), parent_a) != parents_b.end())
			{
				// They share a parent, add sibling relationship
				AddDeityRelationship(deity, other_deity, DeityRelationship::SiblingOf, 1.0f);
	}
		}
	}

	// === Rule 3: Rivalry Emergence ===
	std::vector<DeityHandle> siblings = GetRelatedDeities(deity, DeityRelationship::SiblingOf);
	for (const DeityHandle& sibling : siblings)
	{
		const std::vector<DeityDomain>& sibling_domains = m_deity_data.domains[sibling.index];
		float similarity = SemanticInterceptionBetweenSects(deity_domains, sibling_domains);

		// High similarity = competition = potential rivalry
		float rivalry_probability = similarity * 0.5f; // Scale factor
		if (RandomBetween(0.0f, 1.0f) < rivalry_probability) {
			AddDeityRelationship(deity, sibling, DeityRelationship::EnemyOf, rivalry_probability);
			AddDeityRelationship(sibling, deity, DeityRelationship::EnemyOf, rivalry_probability);
		}
	}

	// === Rule 4: Protector Assignment ===
	for (const DeityHandle& other_deity : pantheon)
	{
		if (other_deity.index == deity.index) {
			continue; // Skip self
		}

		const std::vector<DeityDomain>& other_domains = m_deity_data.domains[other_deity.index];

		// Check if deity's domains complement other's vulnerabilities
		// Ex. War deity protects fertility and innocent deity
		bool is_protector = false;

		// War/Hero Archetypes protect weaker archetypes
		if ((deity_archetype == 6 || deity_archetype == 7) && // Hero or Lover (combat-oriented)
			(m_deity_data.archetype_ids[other_deity.index] == 3 || // Innocent
				m_deity_data.archetype_ids[other_deity.index] == 8)) { // Mother
			is_protector = true;
		}

		// Sage/Magician Protects through Knowledge
		if ((deity_archetype == 2 || deity_archetype == 10) && // Sage or Magician
			std::find(other_domains.begin(), other_domains.end(), DeityDomain::Wisdom) == other_domains.end()) {
			is_protector = true;
		}

		if (is_protector) {
			if (RandomBetween(0.0f, 1.0f) < 0.1f) // low probability
			{
				AddDeityRelationship(deity, other_deity, DeityRelationship::ProtectorOf, 0.8f);
			}
		}
	}

	// === Rule 5: Creator-Servant Relationship ===
	if (deity_archetype == 1) { // Creator archetype
		for (const DeityHandle& other_deity : pantheon)
		{
			if (other_deity.index == deity.index) continue;

			const std::vector<DeityDomain>& other_domains = m_deity_data.domains[other_deity.index];

			// Sercants have derived / lesser domains
			if (other_domains.size() < deity_domains.size() &&
				m_deity_data.prominence_levels[other_deity.index] < m_deity_data.prominence_levels[deity.index])
			{
				if (RandomBetween(0.0f, 1.0f) < 0.3f)
				{
					AddDeityRelationship(deity, other_deity, DeityRelationship::CreatorOf, 0.9f);
					AddDeityRelationship(other_deity, deity, DeityRelationship::ServantOf, 0.9f);
				}
			}
		}
	}
	
	// === Rule 6: Mutilation Myth ===
	if (deity_archetype == 9 || deity_archetype == 5) { // Destroyer or Trickster
		for (const DeityHandle& other_deity : pantheon)
		{
			if (other_deity.index == deity.index) continue;

			// Low probability event
			if (RandomBetween(0.0f, 1.0f) < 0.1f) {
				AddDeityRelationship(deity, other_deity, DeityRelationship::MutilatorOf, 0.7f);

				// The mutilated might become an enemy
				if (RandomBetween(0.0f, 1.0f) < 0.6f)
				{
					AddDeityRelationship(other_deity, deity, DeityRelationship::EnemyOf, 0.9f);
				}
			}
		}
	}

	// === Rule 7: Balance of Power ===
	for (const DeityHandle& other : pantheon) {
		if (other.index == deity.index) continue;

		const std::vector<DeityDomain>& other_domains = m_deity_data.domains[other.index];
		float similarity = SemanticInterceptionBetweenSects(deity_domains, other_domains);

		// Very high similarity without existing relationships = rivalry
		if (similarity > 0.6f)
		{
			// Check if they don't already have a positive relationship
			std::vector<DeityHandle> spouses = GetRelatedDeities(deity, DeityRelationship::SpouseOf);
			std::vector<DeityHandle> parents = GetRelatedDeities(deity, DeityRelationship::ParentOf);

			bool has_positive_relation =
				std::find(spouses.begin(), spouses.end(), other) != spouses.end() ||
				std::find(parents.begin(), parents.end(), other) != parents.end();

			if (!has_positive_relation && RandomBetween(0.0f, 1.0f) < 0.6f) {
				AddDeityRelationship(deity, other, DeityRelationship::EnemyOf, similarity);
				AddDeityRelationship(other, deity, DeityRelationship::EnemyOf, similarity);
			}
		}
	}



}
void ReligionManager::PantheonMerge(ReligionHandle target_religion, ReligionHandle source_religion)
{
	// Merges the pantheon of the source religion into the target religion
}
void ReligionManager::TenetBlending(ReligionHandle target_religion, ReligionHandle source_religion)
{
	// Blends the tenents of the source religion into the target religion
}
void ReligionManager::CleanUpPantheon(ReligionHandle religion_handle)
{
	// Cleans up the pantheon of the specified religion by removing inactive or redundant deities
}
void ReligionManager::TriggerSchism(ReligionHandle parent_religion, Cell& schism_cell)
{
	// Triggers a schism in the parent religion based on the properties of the schism cell
}
void ReligionManager::TriggerReformation(ReligionHandle religion_handle, Cell& reform_cell)
{
	// Triggers a reformation in the specified religion based on the properties of the reform cell
}
void ReligionManager::TriggerRevival(ReligionHandle religion_handle, Cell& revival_cell)
{
	// Triggers a revival of old deities in the specified religion based on the properties of the revival cell
}
void ReligionManager::MutateTenet(ReligionHandle religion_handle)
{
	// Mutates the tenets of the specified religion to introduce variation
}
void ReligionManager::MutateTraits(ReligionHandle religion_handle)
{
	// Mutates the traits of the specified religion to introduce variation
}
void ReligionManager::MutatePantheon(ReligionHandle religion_handle)
{
	// Mutates the pantheon of the specified religion by adding/removing deities or changing relationships
}
void SpawnProphet(ReligionHandle religion_handle, Cell& spawn_cell)
{
	// Spawns a prophet for the specified religion in the given cell
}
void EstablishHolySite(ReligionHandle religion_handle, Cell& site_cell)
{
	// Establishes a holy site for the specified religion in the given cell
}
void SpreadSect(ReligionHandle religion_handle, Cell& target_cell, float influence)
{
	// Spreads the influence of the specified religion into the target cell
}
float ReligionManager::SemanticSimilarity(DeityDomain domain_a, DeityDomain domain_b) const
{
	// Calculates semantic similarity between two deity domains
	return m_definitions.domain_semantic_similarity[static_cast<size_t>(domain_a)][static_cast<size_t>(domain_b)];
}