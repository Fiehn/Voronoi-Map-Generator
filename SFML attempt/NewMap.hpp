#pragma once
#include <filesystem>
#include "Logger.h"

// Struct to wrap the generation steps
struct GenStepWrapper {
    template<typename Func>
    static void RunStep(
        Func&& func,
        const std::string& description,
        int& currentStep,
        int totalSteps,
        sf::RenderWindow& window,
        const sf::Sprite& loadingSprite,
        sf::RectangleShape& loadingBar,
        sf::Text& progressText,
        float maxWidth,
        float maxHeight
    ) {// Execute the function and time it
		LOG_INFO(Generations, "Starting step: %s", description.c_str());
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
		LOG_INFO(Generations, "Completed step: %s", description.c_str());

        // Update progress
        currentStep++;
        float progress = static_cast<float>(currentStep) / totalSteps;

        // Create background loading box
        sf::RectangleShape loadingBox(sf::Vector2f(maxWidth - 200, 50));
        loadingBox.setFillColor(sf::Color::White);
        loadingBox.setPosition(100, maxHeight - 100);

        // Update UI
        loadingBar.setSize(sf::Vector2f(100 + (maxWidth-200) * progress, 50));
        progressText.setString(description + "(" + std::to_string(currentStep) + "/" +
            std::to_string(totalSteps) + ")");

        // Center text
        sf::FloatRect textBounds = progressText.getLocalBounds();
        progressText.setOrigin(textBounds.left + textBounds.width / 2.0f,
            textBounds.top + textBounds.height / 2.0f);
        progressText.setPosition(maxWidth / 2, loadingBar.getPosition().y + 30);

        // Draw
        window.clear();
        window.draw(loadingSprite);
        window.draw(loadingBox);
        window.draw(loadingBar);
        window.draw(progressText);
        window.display();

        // Log timing
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << description << " took: " << duration.count() << "ms\n";
    }
};

static void genWorld(vor::SphereVoronoi& planet,
    GlobalWorldObjects& globals,
    sf::RenderWindow& window,
    sf::VertexArray& lines,
	SeasonalCalculator& seasonCalc,
    const unsigned int MAXWIDTH,
    const unsigned int MAXHEIGHT,
    const sf::Font& font,
    MapConfig& config,
    unsigned int seed,
	PopManager& popManager
) {
    // Seed
    if (seed == 0) { seed = time(NULL); }
    std::srand(seed);


    window.clear();

    // Loading screen elements
    sf::Texture loadingTexture;
    sf::Sprite loadingSprite;
    sf::RectangleShape loadingBar(sf::Vector2f(0, 50));
    sf::Text progressText;

    // Initialize loading elements
    loadingBar.setFillColor(sf::Color::Green);
    loadingBar.setPosition(100, MAXHEIGHT - 100);
    progressText.setFont(font);
    progressText.setFillColor(sf::Color::Black);
    progressText.setCharacterSize(40);
    progressText.setPosition(MAXWIDTH / 2 - progressText.getGlobalBounds().width / 2, MAXHEIGHT / 2 - progressText.getGlobalBounds().height / 2);

    // Load random background image

    // Load a random image from assets/LoadingScreens
    std::vector<std::string> loadingImages;
    for (const auto& entry : std::filesystem::directory_iterator("assets/LoadingScreens")) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
            loadingImages.push_back(entry.path().string());
        }
    }
    if (!loadingImages.empty()) {
        std::string randomImagePath = loadingImages[std::rand() % loadingImages.size()];
        if (loadingTexture.loadFromFile(randomImagePath)) {
            loadingSprite.setTexture(loadingTexture);
            loadingSprite.setPosition(MAXWIDTH / 2 - loadingSprite.getGlobalBounds().width / 2,
                MAXHEIGHT / 2 - loadingSprite.getGlobalBounds().height / 2);
        }
    }
	else {
		std::cout << "No loading images found in assets/LoadingScreens\n";
		progressText.setFillColor(sf::Color::White);
	}

    // Total processing steps
    const int totalSteps = 18;
    int currentStep = 0;

    // Initial draw
    GenStepWrapper::RunStep([]() {}, "Starting generation", currentStep,
        totalSteps, window, loadingSprite, loadingBar,
        progressText, MAXWIDTH, MAXHEIGHT);

    // Globals Clear and Set (Step 1)
    
    GenStepWrapper::RunStep([&]() {
        globals.clearGlobals();
        globals.setSeaLevel(config.sealevel); // RandomBetween(0.4f, 0.6f)
        globals.setGlobalTemp(config.global_temp_avg);
		globals.generatePlanet(config.earthLike);
        globals.generateConvergenceLines();
        lines.clear(); // The convergence lines are drawn on the sphere again with the overlays
        planet.clearMap();
        }, "Initializing Global Variables", currentStep,
        totalSteps, window, loadingSprite, loadingBar,
        progressText, MAXWIDTH, MAXHEIGHT);


    GenStepWrapper::RunStep([&]() {
        planet.fillMap(static_cast<int>(config.n_cells), config.point_jitter);
        const vor::SphereValidation check = vor::validate(planet);
        if (check.ok) { LOG_INFO(Map, "Sphere Voronoi valid:", check.report); }
        else { LOG_ERROR(Map, "Sphere Voronoi failed validation:", check.report); }
        }, "Generating Voronoi diagram", currentStep, totalSteps, window,
        loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);
    // Create the map (Step 2-15)
    auto start = std::chrono::high_resolution_clock::now();

    GenStepWrapper::RunStep([&]() {
        planetHeightGen(planet, globals, config); }, "Generating Heightmap", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

	GenStepWrapper::RunStep([&]() {
		smooth_height(planet.cells, config.rise_threshold, config.height_smooth_repeats,
		config.smooth_method); }, "Smoothing Heightmap", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		planetNoiseHeight(planet, globals, config,
        config.height_noise_repeats); }, "Adding Noise to Heightmap", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcHeightValues(planet.cells, globals, config.delta_coast_line); }, "Finishing Height Map", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		closeOceanCell(planet.cells); }, "Distance To Oceans", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		planetCalcWind(planet, globals); }, "Calculating Wind", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcRiverStart(planet.cells, globals); }, "Calculating River", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		planetCalcTemp(planet, globals); }, "Calculating Temperatures", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		smoothTemps(planet.cells, config.temp_smooth_repeats); }, "Smoothing Temperatures", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		planetCalcPercepitation(planet, globals, config); }, "Calculating Percepetation", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcHumid(planet.cells); }, "Calculating Humidity", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
        planetCalcClimateVariance(planet, globals); }, "Calculating Climate Variance", currentStep,
        totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcBiome(planet.cells, globals.biomes, config.n_ocean_biomes, config.n_land_biomes, config.kmeans_max_iter, config.biome_method, config.prob_smoothing); }, "Calculating Biomes", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		ResourceGen::generateMapResources(planet.cells, globals); }, "Generating Resources", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([]() {}, "Finishing up the map!", currentStep, totalSteps, window,
        loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "Total generation time: " << duration.count() << "ms\n";
    return;
}