#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include <cstdint>

class Culture {
public:
	uint16_t id;
	std::string name;					// Name of the culture
	sf::Color color = sf::Color::White; // Color of the culture

	// Characteristics of the culture
	float inovation_rate = 0.5f;		// 0: tradion, 1: inovative
	float military_tradition = 0.5f;	// 0: pacifist, 1: warlike
	float individualism = 0.5f;			// 0: collectivist, 1: individualist
	float materialism = 0.5f;			// 0: spiritual, 1: materialistic
	float patriarchy = 0.5f;			// 0: matriarchal, 1: patriarchal
	float temperance = 0.5f;			// 0: indulgent, 1: temperate
	float closenessToNature = 0.5f;		// 0: exploitative, 1: nature-loving
	float religiosity = 0.5f;			// 0: secular, 1: religious
	float artFocus = 0.5f;				// 0: utilitarian, 1: artistic
	float socialStratification = 0.5f;	// 0: egalitarian, 1: hierarchical
	float expansionism = 0.5f;			// 0: isolationist, 1: expansionist
	float tradeFocus = 0.5f;			// 0: self-sufficient, 1: trade-oriented
	float multiculturalism = 0.5f;		// 0: xenophobic, 1: multicultural

	// Demographics
	uint64_t total_population = 0; // Total population of the culture
	uint16_t primary_language = 0; // Primary language of the culture (id of the language)

	// Relationships
	uint16_t parent_culture_id = 0; // Parent culture (id of the culture), 0 if none
	std::vector<uint16_t> diverged_cultures; // Cultures that diverged from this culture (id of the culture)

	// Constructor
	Culture(uint16_t id, const std::string& name, const sf::Color& color)
		: id(id), name(name), color(color) {}

	// Calculate cultural distance to another culture
	float CalculateCulturalDistance(const Culture& other) const {
		float distance = 0.0f;
		distance += std::abs(inovation_rate - other.inovation_rate);
		distance += std::abs(military_tradition - other.military_tradition);
		distance += std::abs(individualism - other.individualism);
		distance += std::abs(materialism - other.materialism);
		distance += std::abs(patriarchy - other.patriarchy);
		distance += std::abs(temperance - other.temperance);
		distance += std::abs(closenessToNature - other.closenessToNature);
		distance += std::abs(religiosity - other.religiosity);
		distance += std::abs(artFocus - other.artFocus);
		distance += std::abs(socialStratification - other.socialStratification);
		distance += std::abs(expansionism - other.expansionism);
		distance += std::abs(tradeFocus - other.tradeFocus);
		distance += std::abs(multiculturalism - other.multiculturalism);
		return distance / 10;
	}
};

struct CultureGroup {
	uint16_t id;
	std::string name;
	std::vector<uint16_t> culture_ids; // IDs of cultures in this group

	// Shared characteristics of the culture group
	std::string language_family;
	std::string origin_region;

	CultureGroup(uint16_t id, const std::string& name)
		: id(id), name(name) {}
};



