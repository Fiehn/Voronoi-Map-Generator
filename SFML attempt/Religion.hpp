#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class ReligionType {
	Monotheistic,	// Single deity
	Polytheistic, 	// Multiple deities
	Animistic,		// Spirits in nature
	Pantheistic,	// Deity is identical to the universe
	Atheistic,		// No deities
	Philosophical,	// Focus on philosophy and ethics
	Syncretic,		// Combination of different beliefs
	Humanistic		// Focus on human values and concerns
};

class Religion {
public:
	uint16_t id;
	std::string name;
	ReligionType type;

	// Religious characteristics
	float dogmatism = 0.5f;			// 0: flexible, 1: rigid
	float proselytism = 0.5f;		// 0: non-proselytizing, 1: actively seeking converts
	float ritualism = 0.5f;			// 0: informal, 1: highly ritualistic
	float moral_strictness = 0.5f;	// 0: lenient, 1: strict
	float spirituality = 0.5f;		// 0: secular, 1: deeply spiritual
	float community_focus = 0.5f;	// 0: individualistic, 1: community-oriented
	float mysticism = 0.5f;			// 0: rational, 1: mystical
	float tolerance = 0.5f;			// 0: intolerant, 1: tolerant
	float authoritarianism = 0.5f;	// 0: egalitarian, 1: authoritarian

	// Demographics
	uint64_t total_followers = 0; // Total number of followers

	// Relationships
	uint16_t parent_religion_id = 0; // Parent religion (id of the religion), 0 if none
	std::vector<uint16_t> schisms; // Religions that schismed from this religion (id of the religion)
	std::vector<uint16_t> heretic_sects; // Heretical sects (id of the religion)
	std::vector<uint16_t> allied_religions; // Allied religions (id of the religion)
	std::vector<uint16_t> syncretic_religions; // Religions that have syncretized with this religion (id of the religion)

	// Constructor
	Religion(uint16_t id, const std::string& name, ReligionType type)
		: id(id), name(name), type(type) {}

	// Default constructor
	Religion() : id(0), name("Atheism"), type(ReligionType::Atheistic) {}

	float CalculateCompatibility(const Religion& other) const {
		if (id == other.id) {
			return 1.0f; // Same religion
		}
		float compatibility = 1.0f;

		// Different types have lower compatibility
		if (type != other.type) {
			compatibility -= 0.3f;
		}

		// Calculate differences in characteristics
		compatibility -= std::abs(dogmatism - other.dogmatism) * 0.1f;
		compatibility -= std::abs(proselytism - other.proselytism) * 0.1f;
		compatibility -= std::abs(ritualism - other.ritualism) * 0.1f;
		compatibility -= std::abs(moral_strictness - other.moral_strictness) * 0.1f;
		compatibility -= std::abs(spirituality - other.spirituality) * 0.1f;
		compatibility -= std::abs(community_focus - other.community_focus) * 0.1f;
		compatibility -= std::abs(mysticism - other.mysticism) * 0.1f;
		compatibility -= std::abs(tolerance - other.tolerance) * 0.1f;
		compatibility -= std::abs(authoritarianism - other.authoritarianism) * 0.1f;
		return std::max(0.0f, compatibility);
	}
};

