#pragma once
#include <functional>
#include "PantheonGraphView.hpp"

// Forward declarations
static void pantheonTestWindow(bool& showPantheonTest);

class MenuButton {
public:
    MenuButton(sf::RenderWindow* window, const sf::Font& font, const std::string& text, const sf::Vector2f& size, const sf::Vector2f& position)
        : bwindow(window), bfont(font), btext(text, font, 30), bbox(size), bshadow(size), bsize(size), bposition(position) {
        create_button();
    }

    void create_button() {
        // Adjust text size to fit within the button
        unsigned int charSize = static_cast<unsigned int>(bsize.y * 0.6);
        btext.setCharacterSize(charSize);
        sf::FloatRect textBounds = btext.getLocalBounds();

        // Ensure text fits within the button width
        while (textBounds.width > bsize.x * 0.9f && charSize > 10) {
            charSize -= 2;
            btext.setCharacterSize(charSize);
            textBounds = btext.getLocalBounds();
        }

        // Center text
        btext.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
        btext.setPosition(bposition.x + bsize.x / 2.0f, bposition.y + bsize.y / 2.0f);

        // Set up button appearance
        bbox.setPosition(bposition);
        bbox.setFillColor(sf::Color(200, 200, 200));
        bbox.setOutlineThickness(2);
        bbox.setOutlineColor(sf::Color::Black);

        // Set up shadow
        bshadow.setPosition(bposition.x + 3, bposition.y + 3);
        bshadow.setFillColor(sf::Color(0, 0, 0, 50));
    }

    void draw_button() const {
        bwindow->draw(bshadow);
        bwindow->draw(bbox);
        bwindow->draw(btext);
    }

    bool handle_event(const sf::Event& event) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(*bwindow);
            if (bbox.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos))) {
                if (on_click) on_click();
                return true;
            }
        }
        return false;
    }

    // Callback for click events
    std::function<void()> on_click;

private:
    sf::RenderWindow* bwindow;
    const sf::Font& bfont;
    sf::Text btext;
    sf::RectangleShape bbox;
    sf::RectangleShape bshadow;
    sf::Vector2f bsize;
    sf::Vector2f bposition;
};

bool mainMenu(sf::RenderWindow& window, MapConfig& config, unsigned int MAXWIDTH, unsigned int MAXHEIGHT, bool& showMainMenu)
{
    sf::Font font;
    if (!font.loadFromFile("Fonts/Roboto-Medium.ttf")) { std::cout << "Could not load font" << std::endl; }

    // Screen elements
    sf::Texture menuTexture;
    sf::Sprite menuSprite;
    sf::RectangleShape loadingBar(sf::Vector2f(0, 50));
    sf::Text progressText;

    // Initialize loading elements
    loadingBar.setFillColor(sf::Color::Green);
    loadingBar.setPosition(100, MAXHEIGHT - 100);
    progressText.setFont(font);
    progressText.setFillColor(sf::Color::Black);
    progressText.setCharacterSize(40);
    progressText.setPosition(MAXWIDTH / 2 - progressText.getGlobalBounds().width / 2, MAXHEIGHT / 2 - progressText.getGlobalBounds().height / 2);

    // Create credit text
	sf::Text creditText("Created by: Fiehn", font, 20);
	creditText.setPosition(MAXWIDTH - creditText.getGlobalBounds().width - 10, MAXHEIGHT - creditText.getGlobalBounds().height - 10);

    // Load a random image from assets/MenuScreens
    std::vector<std::string> menuImages;
    for (const auto& entry : std::filesystem::directory_iterator("assets/MenuScreens")) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
            menuImages.push_back(entry.path().string());
        }
    }
    if (!menuImages.empty()) {
        std::string randomImagePath = menuImages[std::rand() % menuImages.size()];
        if (menuTexture.loadFromFile(randomImagePath)) {
            menuSprite.setTexture(menuTexture);
            menuSprite.setPosition(MAXWIDTH / 2 - menuSprite.getGlobalBounds().width / 2,
                MAXHEIGHT / 2 - menuSprite.getGlobalBounds().height / 2);
        }
    }
    else {
        std::cout << "No loading images found in assets/LoadingScreens\n";
        progressText.setFillColor(sf::Color::Black);
    }

    MenuButton randomButton(&window, font, "Generate Random World", sf::Vector2f(300, 80), sf::Vector2f(MAXWIDTH/2 - 150, 250));
    randomButton.on_click = [&]() { std::cout << "Random world starting.." << std::endl; showMainMenu = false; };

	bool showLoadConfig = false;
	bool showSaveConfig = false;
	MenuButton loadConfigFile(&window, font, "Load Config File", sf::Vector2f(300, 80), sf::Vector2f(MAXWIDTH / 2 - 150, 400));
    loadConfigFile.on_click = [&]() { showLoadConfig = true; std::cout << "Load config" << std::endl; };
    
    // Add pantheon test button
    bool showPantheonTest = false;
    MenuButton pantheonTestButton(&window, font, "Test Pantheon Generator", sf::Vector2f(300, 80), sf::Vector2f(MAXWIDTH / 2 - 150, 550));
    pantheonTestButton.on_click = [&]() { showPantheonTest = true; std::cout << "Opening pantheon test" << std::endl; };
    
	bool exit = false;
	MenuButton exitButton(&window, font, "Exit", sf::Vector2f(300, 80), sf::Vector2f(MAXWIDTH / 2 - 150, 700));
    exitButton.on_click = [&]() { std::cout << "Exiting" << std::endl; exit = true; };

    sf::Clock deltaClock;

    while (showMainMenu)
    {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed)
            {
                return false;
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                return false;
            }
            randomButton.handle_event(event);
			loadConfigFile.handle_event(event);
            pantheonTestButton.handle_event(event);
            exitButton.handle_event(event);
        }
        ImGui::SFML::Update(window, deltaClock.restart());

        configLoadSave(config, showLoadConfig, showSaveConfig);
        
        // Handle pantheon test window
        if (showPantheonTest) {
            pantheonTestWindow(showPantheonTest);
        }

        window.clear();

        window.draw(menuSprite);
        window.draw(creditText);

        randomButton.draw_button();
		loadConfigFile.draw_button();
		pantheonTestButton.draw_button();
		exitButton.draw_button();

		if (exit == true) {
            return false;
		}
        
        ImGui::SFML::Render(window);

        window.display();
    }
	return true;
}

// Add pantheon test window function
static void pantheonTestWindow(bool& showPantheonTest) {
    // === Core ECS & Simulation State ===
    static bool initialized = false;
    static flecs::world testWorld;
    static ReligionManager religionManager;
    static History testHistory;
    static flecs::entity testReligion;
    static uint32_t currentTick = 0;
    static vor::Voronoi dummyMap; // Dummy map for the myth phase rule weights

    // Test cell configuration
    static Cell testCell(0);

    // Initialize ECS world and Religion Manager only once
    if (!initialized) {
        // Load domains, archetypes, and religion types into testWorld
        religionManager.initialize(testWorld);
        
        testReligion = testWorld.entity(0);

        // Cell setup
        testCell.height = 0.5f;
        testCell.temp = 20.0f;
        testCell.percepitation = 50.0f;
        testCell.humidity = 0.5f;
        testCell.coastBool = false;
        testCell.riverBool = false;
        testCell.oceanBool = false;
        testCell.distToOcean = 5;
        testCell.windStr = 0.5f;
        testCell.windDir = 180.0f;
        testCell.tempVariance = 10.0f;
        testCell.continent = 0;
        testCell.volcanicActivity = false;

        initialized = true;
    }

    ImGui::Begin("Pantheon Test Generator", &showPantheonTest);

    ImGui::Text("Configure Test Cell Properties:");
    ImGui::Separator();

    // Cell configuration controls
    ImGui::DragFloat("Height", &testCell.height, 0.01f, -1.0f, 2.0f);
    ImGui::DragFloat("Temperature", &testCell.temp, 0.5f, -50.0f, 50.0f);
    ImGui::DragFloat("Precipitation", &testCell.percepitation, 1.0f, 0.0f, 200.0f);
    ImGui::DragFloat("Humidity", &testCell.humidity, 0.01f, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Geographical Features:");
    ImGui::Checkbox("Ocean", &testCell.oceanBool); ImGui::SameLine();
    ImGui::Checkbox("Coast", &testCell.coastBool); ImGui::SameLine();
    ImGui::Checkbox("River", &testCell.riverBool);
    ImGui::Checkbox("Volcanic Activity", &testCell.volcanicActivity);
    ImGui::DragInt("Distance to Ocean", &testCell.distToOcean, 1.0f, 0, 100);

    ImGui::Separator();
    ImGui::Text("Climate Variance:");
    ImGui::DragFloat("Temp Variance", &testCell.tempVariance, 0.1f, 0.0f, 30.0f);
    ImGui::DragFloat("Wind Strength", &testCell.windStr, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Wind Direction", &testCell.windDir, 1.0f, 0.0f, 360.0f);

    ImGui::Separator();

    // Note: Removed the Religion Type ImGui Combo because your new create_proto_religion() 
    // automatically weights and assigns the type using determine_proto_type().

    // Static graph viewer initialized with our test world
    static PantheonGraphView graphView(&testWorld);

    // === Generation Controls ===
    if (ImGui::Button("Generate Proto-Religion", ImVec2(200, 40))) {
        // Reset state
        testHistory.clear();
        currentTick = 0;

        // Clean up previous test religion if it exists to keep the ECS graph clean
        if (testReligion.id() != 0 && testReligion.is_alive()) {
            testReligion.destruct();
        }

        // Generate!
        religionManager.create_proto_religion(testWorld, testHistory, testCell);

        // Find the newly generated religion in the ECS world
        testWorld.query_builder<Religion>()
            .without<Extinct>()
            .build()
            .each([&](flecs::entity rel, const Religion& r) {
            testReligion = rel;
                });

        // Bind and initialize graph
        graphView.SetReligion(testReligion);
    }

    // === Step Controls ===
    if (testReligion.id() != 0 && testReligion.is_alive()) {
        ImGui::SameLine();
        if (ImGui::Button("Step Myth Phase (1 Tick)", ImVec2(200, 40))) {
            currentTick++;
            religionManager.myth_phase_evolution(testWorld, testHistory, testReligion, currentTick, dummyMap);

            // Re-initialize graph to catch newly created/destroyed deities
            graphView.InitializeGraph();
        }
    }

    ImGui::End();

    // === Render Graph and History Log ===
    if (testReligion.id() != 0 && testReligion.is_alive()) {
        // Draw the visual node graph
        graphView.Draw();

        // Draw the History Log
        ImGui::Begin("Chronicle of History");
        if (ImGui::Button("Clear History")) testHistory.clear();
        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& ev : testHistory.events) {

            // Color code the events based on type
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White default
            if (ev.type == EventType::DeityCreation) color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Green
            if (ev.type == EventType::DeityInteraction) color = ImVec4(0.5f, 0.8f, 1.0f, 1.0f); // Light blue
            // Note: Add DivineConflict, Schism, etc., here once you update the enum!

            ImGui::PushStyleColor(ImGuiCol_Text, color);

            std::string causal_link = ev.caused_by_event_id ? " [Caused by Event #" + std::to_string(*ev.caused_by_event_id) + "]" : "";

            ImGui::TextWrapped("[Tick %d] Event #%llu: %s %s %s. (Reason: %s)%s",
                ev.tick,
                ev.event_id,
                ev.snapshot.subject_name.c_str(),
                ev.snapshot.location_name.c_str(),
                ev.snapshot.object_name.c_str(),
                ev.snapshot.reason.c_str(),
                causal_link.c_str());

            ImGui::PopStyleColor();
        }

        // Auto-scroll logic
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }
}