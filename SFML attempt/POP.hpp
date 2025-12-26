#pragma once
#include "Culture.hpp"
#include "Religion.hpp"
#include "Faction.hpp"

class State
{
public:
	int id;
	std::string name;
	std::vector<Cell> cells;
	// WHooole lotta shit
};


enum struct ProfessionType
{
	None,
	Farmer,
	Hunter,
	Fisher,
	Merchant,
	Artisan,
	Scholar,
	Soldier,
	Cleric,
	Nomad,
	Aristocrat,
	Slave,
	Capitalist
};

struct Value {
	std::string name; // Name of the value
	std::string description; // Description of the value
	float influence; // Influence of the value on the culture (0 to 1) (1 being the most influence)
};

struct Idea {
	std::string name;
	std::string description; // Description of the idea
	float influence; // Influence of the idea on the culture (0 to 1) (1 being the most influence)
};

struct EconomicStatus {
	float wealth; // Wealth of the culture (0 to 1) (1 being the richest)
	float needs; // Needs of the culture (0 to 1) (1 being the most needs)
	float employment; // Employment of the culture (0 to 1) (1 being the most employed)
	ProfessionType profession;
};

struct SocialStatus {
	float happiness; // Happiness of the culture (0 to 1) (1 being the happiest)
	float literacy; // Education of the culture (0 to 1) (1 being the most educated)
	float health; // Health of the culture (0 to 1) (1 being the healthiest)
	float crime; // Crime of the culture (0 to 1) (1 being the most crime)
	float population; // Population of the culture (0 to 1) (1 being the most populated)
};

struct Demographic {
	float birthRate; // Birth rate of the culture (0 to 1) (1 being the most births)
	float deathRate; // Death rate of the culture (0 to 1) (1 being the most deaths)
	float migrationRate; // Migration rate of the culture (0 to 1) (1 being the most migration)
	float lifeExpectancy; // Life expectancy of the culture (0 to 1) (1 being the most life expectancy)
};


class POP
{
	std::size_t id; // Unique Id of the POP
	std::size_t cultureId; // Culture Id of the POP
	std::size_t religionId; // Religion Id of the POP
	std::size_t stateId; // State Id of the POP
	std::vector<Value> values; // Values of the POP
	std::vector<Idea> ideas; // Ideas of the POP
	EconomicStatus economicStatus; // Economic status of the POP
	SocialStatus socialStatus; // Social status of the POP
	Demographic demographic; // Demographic of the POP
	std::size_t population; // Population of the POP
	std::int32_t flags; // Pack out the bitfield for the flags (0 to 31)
	std::map<Faction*, float> politicalSupport; // Faction -> Support (-1 to 1) (1 being the most support)
	std::map<State*, float> stateSupport; // State -> Support (-1 to 1) (1 being the most support)

	std::size_t cellId; // Cell Id of the POP (the cell that the POP is in)

	void updateValues(); // Update the values of the POP
};

// Each cell then has a vector of POPs, this is a nightmare to manage


