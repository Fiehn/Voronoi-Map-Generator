#pragma once
#include <filesystem>

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
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();

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

static void genWorld(vor::Voronoi& map,
    GlobalWorldObjects& globals,
    sf::RenderWindow& window,
    VertexMap& vertexMap,
    sf::VertexArray& lines,
	SeasonalCalculator& seasonCalc,
    const unsigned int MAXWIDTH,
    const unsigned int MAXHEIGHT,
    const sf::Font& font,
    MapConfig& config,
    unsigned int seed
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
    const int totalSteps = 22;
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
		int numLines = globals.convergenceLines.size();
        lines.clear();
        lines.resize(2 * numLines);
        for (int i = 0; i < numLines * 2; i++)
        {
            if (i % 2 == 0)
            {
                lines.append(sf::Vertex(sf::Vector2f(0, globals.convergenceLines[std::floor(i / 2)] * MAXHEIGHT), sf::Color::Green));
            }
            else
            {
                lines.append(sf::Vertex(sf::Vector2f(MAXWIDTH, globals.convergenceLines[std::floor(i / 2)] * MAXHEIGHT), sf::Color::Green));
            }
        }
        std::vector<sf::Color> biomeColors = randomColors(config.n_biomes);
        for (int i = 0; i < config.n_biomes; i++) {
            globals.addBiome("Biome" + std::to_string(i), biomeColors[i]);
        }
        biomeColors.clear();
        map.clearMap();
        }, "Initializing Global Variables", currentStep,
        totalSteps, window, loadingSprite, loadingBar,
        progressText, MAXWIDTH, MAXHEIGHT);


    GenStepWrapper::RunStep([&]() {
        map.fillMap(config.ncellx, config.ncelly, MAXWIDTH, MAXHEIGHT,
        config.point_jitter);
        }, "Generating Voronoi diagram", currentStep, totalSteps, window,
        loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);
    // Create the map (Step 2-15)
    auto start = std::chrono::high_resolution_clock::now();

    GenStepWrapper::RunStep([&]() {
        random_height_gen(map.cells, globals, config, map.points, map.voronoi_points); }, "Generating Heightmap", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);
 
	GenStepWrapper::RunStep([&]() {
		smooth_height(map.cells, config.rise_threshold, config.height_smooth_repeats,
		config.smooth_method); }, "Smoothing Heightmap", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		noise_height(map.cells, globals, config, map.points,
        config.height_noise_repeats); }, "Adding Noise to Heightmap", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcHeightValues(map.cells, globals, config.delta_coast_line); }, "Finishing Height Map", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		closeOceanCell(map.cells, globals); }, "Distance To Oceans", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcWind(map.cells, map.points, MAXHEIGHT, globals); }, "Calculating Wind", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcRiverStart(map.cells, globals, map.points, map.voronoi_points); }, "Calculating River", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcTemp(map.cells, globals, map.points, MAXHEIGHT); }, "Calculating Temperatures", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		smoothTemps(map.cells, config.temp_smooth_repeats); }, "Smoothing Temperatures", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcPercepitation(map.cells, map.points, globals, config.percepitation_repeats, config.max_percipitation,config.ocean_evaporation_factor,config.land_evapotranspiration_factor, config.moisture_loss_rate, config.orographic_factor, config.moisture_loss_rate); }, "Calculating Percepetation", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcHumid(map.cells); }, "Calculating Humidity", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
        calcClimateVariance(map.cells, map.points, globals, (float)MAXHEIGHT); }, "Calculating Climate Variance", currentStep,
        totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		calcBiome(map.cells, globals, config.kmeans_max_iter, config.biome_method); }, "Calculating Biomes", currentStep, 
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		ResourceGen::generateMapResources(map.cells, globals); }, "Generating Resources", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
        globals.initializeCultures(map.cells); }, "Initializing Cultures", currentStep,
        totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([&]() {
		vertexMap.clear();
		vertexMap.create(map);
		vertexMap.genVertexMap(map); }, "Generating Vertex Buffer", currentStep,
		totalSteps, window, loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

    GenStepWrapper::RunStep([]() {}, "Finishing up the map!", currentStep, totalSteps, window,
        loadingSprite, loadingBar, progressText, MAXWIDTH, MAXHEIGHT);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "Total generation time: " << duration.count() << "ms\n";
    return;
}