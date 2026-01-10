#include "POP_generator.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

PopGenerator::PopGenerator(unsigned int seed)
{
	if (seed == 0) {
		std::random_device rd;
		seed = rd();
	}
	m_rng.seed(seed);
}

// ===== Culture Generation =====
std::vector<Culture> PopGenerator::GenerateCultures(const PopGenerationConfig& config, const vor::Voronoi& map)
{
	std::vector<Culture> cultures;

	// Determine number of cultures to generate
	int num_cultures = std::uniform_int_distribution<int>(config.min_cultures, config.max_cultures)(m_rng);
	cultures.reserve(num_cultures);

	// Generate Colors
	ColorTable colorTable;
	std::vector<sf::Color> cultureColors = colorTable.getRandomColors(num_cultures);

	for (int i = 0; i < num_cultures; ++i) {
		std::string name = GenerateName("culture");
		Culture culture(static_cast<uint16_t>(i), name, cultureColors[i]);
		
		GenerateCulturalTraits(culture);

		cultures.push_back(culture);
	}
	return cultures;
}

void PopGenerator::GenerateCulturalTraits(Culture& culture)
{
	culture.inovation_rate = betaDist(2.0f, 2.0f);
	culture.military_tradition = betaDist(2.0f, 2.0f);
	culture.individualism = betaDist(2.0f, 2.0f);
	culture.materialism = betaDist(2.0f, 2.0f);
	culture.patriarchy = betaDist(2.0f, 2.0f);
	culture.temperance = betaDist(2.0f, 2.0f);
	culture.closenessToNature = betaDist(2.0f, 2.0f);
	culture.religiosity = betaDist(2.0f, 2.0f);
	culture.artFocus = betaDist(2.0f, 2.0f);
	culture.socialStratification = betaDist(2.0f, 2.0f);
	culture.expansionism = betaDist(2.0f, 2.0f);
	culture.tradeFocus = betaDist(2.0f, 2.0f);
	culture.multiculturalism = betaDist(2.0f, 2.0f);
}

// ===== Religion Generation =====
std::vector<Religion> PopGenerator::GenerateReligions(const PopGenerationConfig& config, const vor::Voronoi& map)
{
	std::vector<Religion> religions;
	// Determine number of religions to generate
	int num_religions = std::uniform_int_distribution<int>(config.min_religions, config.max_religions)(m_rng);
	religions.reserve(num_religions);

	// Distribution of Religion Types
	std::vector<ReligionType> types = {
		ReligionType::Monotheistic,
		ReligionType::Polytheistic,
		ReligionType::Animistic,
		ReligionType::Pantheistic,
		ReligionType::Philosophical,
		ReligionType::Syncretic,
		ReligionType::Humanistic
	};

	for (int i = 0; i < num_religions; ++i) {
		ReligionType type;
		if (i == 0) {
			type = ReligionType::Animistic; // First religion is Animistic by default
		}
		else {
			type = types[std::uniform_int_distribution<int>(0, types.size() - 1)(m_rng)];
		}
		std::string name = GenerateName("religion");
		Religion religion(static_cast<uint16_t>(i), name, type);

		GenerateReligiousTraits(religion);

		religions.push_back(religion);
	}
	return religions;
}

void PopGenerator::GenerateReligiousTraits(Religion& religion)
{
	religion.dogmatism = betaDist(2.0f, 2.0f);
	religion.proselytism = betaDist(2.0f, 2.0f);
	religion.ritualism = betaDist(2.0f, 2.0f);
	religion.moral_strictness = betaDist(2.0f, 2.0f);
	religion.spirituality = betaDist(2.0f, 2.0f);
	religion.community_focus = betaDist(2.0f, 2.0f);
	religion.mysticism = betaDist(2.0f, 2.0f);
	religion.tolerance = betaDist(2.0f, 2.0f);
	religion.authoritarianism = betaDist(2.0f, 2.0f);
}

// ===== Language Generation =====
std::vector<Language> PopGenerator::GenerateLanguages(const PopGenerationConfig& config, const vor::Voronoi& map)
{
	std::vector<Language> languages;
	// Determine number of languages to generate
	int num_languages = std::uniform_int_distribution<int>(config.min_languages, config.max_languages)(m_rng);
	languages.reserve(num_languages);

	int num_parent_languages = std::max(1, num_languages / 4); // At least one parent language, up to a quarter of total languages

	for (int i = 0; i < num_parent_languages; ++i) {
		std::string name = GenerateName("language");
		Language language(static_cast<uint16_t>(i), name);
		GenerateLinguisticTraits(language);
		languages.push_back(language);
	}

	// Create derived languages
	for (int i = static_cast<int>(languages.size()); i < num_languages; ++i) {
		size_t parent_index = std::uniform_int_distribution<size_t>(0, i - 1)(m_rng);
		Language& parent_language = languages[parent_index];

		std::string name = GenerateName("language");
		Language language(static_cast<uint16_t>(i), name);
		GenerateLinguisticTraits(language);

		// inherit some traits from parent language
		language.grammatical_complexity = (language.grammatical_complexity + parent_language.grammatical_complexity) / 2.0f;
		language.lexical_richness = (language.lexical_richness + parent_language.lexical_richness) / 2.0f;
		language.phonetic_complexity = (language.phonetic_complexity + parent_language.phonetic_complexity) / 2.0f;
		language.parent_language_id = parent_language.id;

		languages.push_back(language);
		parent_language.derived_languages.push_back(language.id);
	}

	return languages;
}

void PopGenerator::GenerateLinguisticTraits(Language& language)
{
	language.phonetic_complexity = betaDist(2.0f, 2.0f);
	language.prestige = betaDist(2.0f, 2.0f);
	language.attractiveness = betaDist(2.0f, 2.0f);
	language.grammatical_complexity = betaDist(2.0f, 2.0f);
	language.lexical_richness = betaDist(2.0f, 2.0f);
}

// ===== Population Generation =====

void PopGenerator::GenerateInitialPopulation(PopManager& popManager,
	const PopGenerationConfig& config,
	const vor::Voronoi& map,
	const GlobalWorldObjects& globals)
{
	// ensure spatial index is initialized
	popManager.InitializeSpatialIndex(map.cells.size());

	// Get cultures, religions, and languages from globals
	// These are created prior to population generation and passed in via globals
	const auto& cultures = globals.cultures;
	const auto& religions = globals.religions;
	const auto& languages = globals.languages;

	if (cultures.empty() || religions.empty() || languages.empty()) {
		throw std::runtime_error("Cultures, Religions, and Languages must be generated before generating population.");
	}

	// Create pops for each land cell
	for (size_t cell_idx = 0; cell_idx < map.cells.size(); ++cell_idx) {
		const Cell& cell = map.cells[cell_idx];
		if (cell.oceanBool) {
			continue; // Skip ocean cells
		}

		int32_t cell_population = GetCellPopulation(cell, config);
		if (cell_population <= 0) {
			continue; // No population to generate
		}
		
		// Create POPs for the cell
		int32_t remaining_population = cell_population;
		while (remaining_population > 0) {
			// using exponential distribution to create POP sizes (so the first POPs are larger, and the rest smaller) 
			// using 1000 as the mean size TODO: adjust this based on cell size/population density
			int32_t pop_size = static_cast<int32_t>(std::round(std::exponential_distribution<float>(1.0f / (cell_population * (1 - config.culture_mixing_rate)))(m_rng)));
			if (pop_size <= 0) {
				pop_size = 1;
			}
			if (pop_size > remaining_population) {
				pop_size = remaining_population;
			}
			remaining_population -= pop_size;

			// Determine culture, religion, and language for the POP
			uint16_t culture_id = DetermineCellCulture(cell_idx, cell, cultures, map);
			uint16_t religion_id = DeterminePopReligion(cell_idx, cell, religions, map, culture_id);
			// Language is determined by culture's primary language (for simplicity) TODO: change this later
			uint16_t language_id = cultures[culture_id].primary_language;
			// Create the POP
			PopCreateInfo popInfo;
			popInfo.province_id = cell_idx;
			popInfo.initial_count = pop_size;
			popInfo.culture_id = culture_id;
			popInfo.religion_id = religion_id;
			popInfo.language_id = language_id;
			popInfo.job_id = -1; // Unemployed by default TODO: assign jobs later

			// intial values for economic and social parameters
			popInfo.initial_wealth = 50.0f + cell.height * 100.0f + normalDistPDF(0.0f, config.wealth_variance * 50.0f);
			popInfo.initial_literacy = std::clamp(config.literacy_base + normalDistPDF(0.0f, config.literacy_variance), 0.0f, 1.0f);
			popInfo.initial_concious = betaDist(2.0f, 5.0f); // more traditional by default
			popInfo.initial_happy = 0.5f + (cell.temp / 40.0f) * 0.2f; // happier in moderate temps
			popInfo.initial_militancy = betaDist(2.0f, 8.0f); // generally low militancy

			popManager.CreatePop(popInfo);
		}
	}
}

int32_t PopGenerator::GetCellPopulation(const Cell& cell, const PopGenerationConfig& config)
{
	if (cell.oceanBool) {
		return 0; // No population in ocean cells
	}

	float pop = config.population_density; // base population

	float height_factor = 1.0f - std::abs(cell.height - 0.5f) * 2.0f; // prefer mid-height land
	pop *= height_factor;
	float temp_factor = 1.0f - std::abs(cell.temp - 15.0f) / 30.0f; // prefer moderate temperatures
	pop *= 0.5f + temp_factor;

	pop *= 0.5f + (cell.humidity * 0.5f); // prefer humid areas

	if (cell.coastBool) {
		pop *= config.coastal_preference; // boost for coastal cells
	}

	// river boost
	if (cell.riverBool) {
		pop *= 1.4f;
	}

	pop *= std::uniform_real_distribution<float>(0.8f, 1.2f)(m_rng); // random variation
	return static_cast<int32_t>(std::round(pop * 1000.0f)); // scale to get population count
}

uint16_t PopGenerator::DetermineCellCulture(size_t cellIndex,
	const Cell& cell,
	const std::vector<Culture>& cultures,
	const vor::Voronoi& map)
{
	// TODO: implement more complex culture determination logic based on neighboring cells, culture mixing, etc.
	
	// For simplicity, assign culture based on cell's culture attribute if valid
	if (cell.culture >= 0 && static_cast<size_t>(cell.culture) < cultures.size()) {
		return static_cast<uint16_t>(cell.culture);
	}
	// Fallback: random culture
	return static_cast<uint16_t>(std::uniform_int_distribution<size_t>(0, cultures.size() - 1)(this->m_rng));
}

uint16_t PopGenerator::DeterminePopReligion(size_t cellIndex,
	const Cell& cell,
	const std::vector<Religion>& religions,
	const vor::Voronoi& map,
	uint16_t culture_id)
{
	// TODO: implement more complex religion determination logic based on culture, neighboring cells, conversion rates, etc.
	return static_cast<uint16_t>(std::uniform_int_distribution<size_t>(0, religions.size() - 1)(m_rng));
}

std::string PopGenerator::GenerateName(const std::string& type)
{
	// TODO: NAME GENERATION WILL BE A FUTURE IMPROVEMENT
	
	// Simple name generator using random syllables
	static const std::vector<std::string> syllables = {
		"an", "ar", "ba", "be", "bi", "bo", "da", "de", "di", "do",
		"el", "en", "er", "fa", "fi", "fo", "ga", "ge", "gi", "go",
		"ha", "he", "hi", "ho", "il", "in", "ir", "ja", "je", "ji",
		"ka", "ke", "ki", "ko", "la", "le", "li", "lo", "ma", "me",
		"mi", "mo", "na", "ne", "ni", "no", "ol", "on", "or", "pa",
		"pe", "pi", "po", "ra", "re", "ri", "ro", "sa", "se", "si",
		"so", "ta", "te", "ti", "to", "ul", "un", "ur", "va", "ve",
		"vi", "vo", "za", "ze", "zi", "zo"
	};
	int syllable_count = std::uniform_int_distribution<int>(2, 4)(m_rng);
	std::string name;
	for (int i = 0; i < syllable_count; ++i) {
		name += syllables[std::uniform_int_distribution<size_t>(0, syllables.size() - 1)(m_rng)];
	}
	// Capitalize first letter
	name[0] = static_cast<char>(std::toupper(name[0]));
	return name;
}

