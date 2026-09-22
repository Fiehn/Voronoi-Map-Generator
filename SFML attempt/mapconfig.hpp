#pragma once
#include <fstream>
#include "Include/nlohmann/json.hpp"
using json = nlohmann::json;
#include <chrono>
#include <iostream>

class MapConfig {
public:
	// constructor
	MapConfig() = default;

	// Point generation
	float point_jitter = 7.f; // Jitter for the points
	unsigned int ncellx = 150; // Number of cells in x direction
	unsigned int ncelly = 150; // Number of cells in y direction

    // Height generation
    unsigned int npeaks = 10; // Number of peaks to generate in the heightmap / continents
    float delta_max_neg = 0.04f; // The maximum amount of random height added in the negative direction
    float delta_max_pos = 0.02f; // The maximum amount of random height added in the positive direction
    float prob_of_island = 0.002f; // small probability of random height increase when away from mainland 
    float dist_from_mainland = 1.0f; // The distance from the mainland where the probability of random height increase begins, Represented by the sum of height of all neighbors
    int height_method = 2; // Method 1 is k-peaks without continent interaction, method 2 is k-peaks with continent interaction, 3 is continent based generation
    float rise_threshold = 0.25f; // The minimum rise value where a cell height is smoothed 
    unsigned int height_smooth_repeats = 1; // amount of height smoothing iterations
    int smooth_method = 1; // method 1 is random the other is front
	int heigth_noise_method = 1; // method 1 is random noise, method 2 is simplex noise
    unsigned int height_noise_repeats = 1; // amount of height noise iterations, happens after smoothing
    float delta_coast_line = 0.05f; // the range around sealevel that is considered coast (below and above)

	float oceanic_plate_ratio = 0.5f; // ratio of oceanic plates vs continental plates
	float continental_crust_thickness = 35.0f; // thickness of continental crust
	float oceanic_crust_thickness = 7.0f; // thickness of oceanic crust
	bool enable_erosion = true; // enable erosion simulation
	int erosion_iterations = 0; // number of erosion simulation iterations
	float erosion_strength = 0.15f; // strength of erosion effect

    // Temperature
    float global_temp_avg = 35.f; // not the actual average but a value that determines the temperature range
    unsigned int temp_smooth_repeats = 0; // amount of temperature smoothing iterations

    // Sealevel
    float sealevel = 0.5f; // The height at which the ocean starts

    // Percepitation
    unsigned int percepitation_repeats = 3; // amount of percepitation iterations (NEEDs to be above 1)
	float max_percipitation = 300.0f; // cap
	float ocean_evaporation_factor = 1.0f; // multiplier for ocean evaporation
	float land_evapotranspiration_factor = 0.6f; // multiplier for land evapotranspiration
	float moisture_loss_rate = 0.02f; // amount of moisture lost per cell
	float orographic_factor = 3.0f; // height influence on percepitation
	float condensation_rate = 0.15f; // rate at which moisture condenses into precipitation

    // Biomes
    unsigned int kmeans_max_iter = 5; // The maximum amount of iterations for the kmeans algorithm
    unsigned int n_biomes = 8; // The amount of clusters for the kmeans algorithm (amount of biomes)
    unsigned int biome_method = 1; // Method 1 is GMM and method 2 is Kmeans
    float prob_smoothing = 0.5f;

    // Planet Parameters
	bool earthLike = false; // if true, sets parameters to earth like values
    
	// POP Generation Config
	float population_density = 1.0f; // Multiplier for population density
	float urbanization_rate = 0.3f; // Percentage of population in urban areas
	float coastal_preference = 1.2f; // Multiplier for coastal population preference
	int min_cultures = 5;		// Minimum number of cultures
	int max_cultures = 20;		// Maximum number of cultures
	float culture_mixing_rate = 0.1f; // Rate of cultural mixing
	int min_religions = 3;		// Minimum number of religions
	int max_religions = 15;		// Maximum number of religions
	float religious_conversion_rate = 0.05f; // Rate of religious conversion
	int min_languages = 4;		// Minimum number of languages
	int max_languages = 25;		// Maximum number of languages
	float language_shift_rate = 0.07f; // Rate of language shift
	float wealth_variance = 0.3f; // Variance in wealth distribution
	float literacy_base = 0.2f;   // Base literacy rate
	float literacy_variance = 0.25f; // Variance in literacy rates
	float coastal_population_preference = 1.2f; // Multiplier for coastal population preference

	// JSON functions
	void save_json(const std::string& filename) const {
		json j;
		to_json(j);
		std::ofstream o(filename);
		o << std::setw(4) << j << std::endl;
	}

	void load_json(const std::string& filename) {
		json j;
		std::ifstream i(filename);
		i >> j;
		from_json(j);
	}
private:
	void to_json(json& j) const {
		j = json{
			{"point_jitter", point_jitter},
			{"ncellx", ncellx},
			{"ncelly", ncelly},
			{"npeaks", npeaks},
			{"delta_max_neg", delta_max_neg},
			{"delta_max_pos", delta_max_pos},
			{"prob_of_island", prob_of_island},
			{"dist_from_mainland", dist_from_mainland},
			{"height_method", height_method},
			{"rise_threshold", rise_threshold},
			{"height_smooth_repeats", height_smooth_repeats},
			{"smooth_method", smooth_method},
			{"height_noise_repeats", height_noise_repeats},
			{"delta_coast_line", delta_coast_line},
			{"global_temp_avg", global_temp_avg},
			{"temp_smooth_repeats", temp_smooth_repeats},
			{"sealevel", sealevel},
			{"percepitation_repeats", percepitation_repeats},
			{"kmeans_max_iter", kmeans_max_iter},
			{"n_biomes", n_biomes},
			{"biome_method", biome_method},
			{"prob_smoothing", prob_smoothing},
			{"max_percipitation", max_percipitation},
			{"moisture_loss_rate", moisture_loss_rate},
			{"orographic_factor", orographic_factor},
			{"land_evapotranspiration_factor", land_evapotranspiration_factor},
			{"ocean_evaporation_factor", ocean_evaporation_factor},
			{"condensation_rate", condensation_rate},
			{"earthLike", earthLike},
			{"population_density", population_density},
			{"urbanization_rate", urbanization_rate},
			{"coastal_preference", coastal_preference},
			{"min_cultures", min_cultures},
			{"max_cultures", max_cultures},
			{"culture_mixing_rate", culture_mixing_rate},
			{"min_religions", min_religions},
			{"max_religions", max_religions},
			{"religious_conversion_rate", religious_conversion_rate},
			{"min_languages", min_languages},
			{"max_languages", max_languages},
			{"language_shift_rate", language_shift_rate},
			{"wealth_variance", wealth_variance},
			{"literacy_base", literacy_base},
			{"literacy_variance", literacy_variance},
			{"coastal_population_preference", coastal_population_preference} 
		};
	}

	void from_json(json& j) {
		point_jitter = j.at("point_jitter").get<float>();
		ncellx = j.at("ncellx").get<unsigned int>();
		ncelly = j.at("ncelly").get<unsigned int>();
		npeaks = j.at("npeaks").get<unsigned int>();
		delta_max_neg = j.at("delta_max_neg").get<float>();
		delta_max_pos = j.at("delta_max_pos").get<float>();
		prob_of_island = j.at("prob_of_island").get<float>();
		dist_from_mainland = j.at("dist_from_mainland").get<float>();
		height_method = j.at("height_method").get<int>();
		rise_threshold = j.at("rise_threshold").get<float>();
		height_smooth_repeats = j.at("height_smooth_repeats").get<unsigned int>();
		smooth_method = j.at("smooth_method").get<int>();
		height_noise_repeats = j.at("height_noise_repeats").get<unsigned int>();
		delta_coast_line = j.at("delta_coast_line").get<float>();
		global_temp_avg = j.at("global_temp_avg").get<float>();
		temp_smooth_repeats = j.at("temp_smooth_repeats").get<unsigned int>();
		sealevel = j.at("sealevel").get<float>();
		percepitation_repeats = j.at("percepitation_repeats").get<unsigned int>();
		kmeans_max_iter = j.at("kmeans_max_iter").get<unsigned int>();
		n_biomes = j.at("n_biomes").get<unsigned int>();
		biome_method = j.at("biome_method").get<int>();
		prob_smoothing = j.at("prob_smoothing").get<float>();
		max_percipitation = j.at("max_percipitation").get<float>();
		moisture_loss_rate = j.at("moisture_loss_rate").get<float>();
		orographic_factor = j.at("orographic_factor").get<float>();
		land_evapotranspiration_factor = j.at("land_evapotranspiration_factor").get<float>();
		ocean_evaporation_factor = j.at("ocean_evaporation_factor").get<float>();
		condensation_rate = j.at("condensation_rate").get<float>();
		earthLike = j.at("earthLike").get<bool>();
		population_density = j.at("population_density").get<float>();
		urbanization_rate = j.at("urbanization_rate").get<float>();
		coastal_preference = j.at("coastal_preference").get<float>();
		min_cultures = j.at("min_cultures").get<int>();
		max_cultures = j.at("max_cultures").get<int>();
		culture_mixing_rate = j.at("culture_mixing_rate").get<float>();
		min_religions = j.at("min_religions").get<int>();
		max_religions = j.at("max_religions").get<int>();
		religious_conversion_rate = j.at("religious_conversion_rate").get<float>();
		min_languages = j.at("min_languages").get<int>();
		max_languages = j.at("max_languages").get<int>();
		language_shift_rate = j.at("language_shift_rate").get<float>();
		wealth_variance = j.at("wealth_variance").get<float>();
		literacy_base = j.at("literacy_base").get<float>();
		literacy_variance = j.at("literacy_variance").get<float>();
		coastal_population_preference = j.at("coastal_population_preference").get<float>();
	}
};


