#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <cstdlib>
#include <chrono>

#include "Logger.h"

#include "util.hpp"

#include "mapconfig.hpp"
#include "windArrows.hpp"
#include "Voronoi.hpp"
#include "vertex.hpp"
#include "cell.hpp"
#include "GlobalWorldObjects.hpp"
#include "Map.hpp"
#include "POP_Manager.hpp"
#include "SeasonalCalculator.hpp"

#include "imgui.h"
#include "imgui-SFML.h"
#include "Include/ImGuiFD-main/ImGuiFD.h"
#include "Include/ImPlot/implot.h"
#include "GLContext.hpp"
#include "SphereVoronoi.hpp"
#include "PlanetView.hpp"
#include "PlanetColors.hpp"
#include "NewMap.hpp"
#include "EventMachine.hpp"
#include "MainMenu.hpp"

#include "tickerSimulation.hpp"


int main() 
{
    // Initate seed and window size
    std::srand(time(NULL));
    unsigned int seed = 0; // for changing the seed
    unsigned int windowWidth = 2500;
    unsigned int windowHeight = 1500;

    // MSAA so the smoothed boundaries and outlines don't stair-step
    sf::ContextSettings contextSettings;
	contextSettings.depthBits = 24;
	contextSettings.majorVersion = 3;
	contextSettings.minorVersion = 3;
    contextSettings.antialiasingLevel = 4;
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "SFML", sf::Style::Default, contextSettings);
    window.setVerticalSyncEnabled(true); // The globe needs smooth orbiting
    ImGui::SFML::Init(window);
	ImPlot::CreateContext();

	initGL();
	LOG_INFO(System, "Depth bits:", static_cast<int>(window.getSettings().depthBits));

    // Create the config
    MapConfig config;

    bool showMainMenu = true;

    if (!mainMenu(window, config, windowWidth, windowHeight, showMainMenu)) 
    {
		window.close();
		ImGui::SFML::Shutdown(window);
		return 0;
    }
    
    // Create the global world objects
    GlobalWorldObjects globals;

    // Empty additionals
    sf::VertexArray windArrows;
    sf::VertexArray lines;
    lines.setPrimitiveType(sf::Lines);

    // The cell shown in the Highlighted Cell panel, the hovered one while Highlight Cell is on
    std::size_t highlightedCell = vor::INVALID_INDEX;

    // Load font!
    sf::Font font;
    if (!font.loadFromFile("Fonts/Roboto-Medium.ttf")) { std::cout << "Could not load font" << std::endl; }

    // The planet: generated on the sphere and shown on the globe
    vor::SphereVoronoi planet;
    PlanetView planetView;

    // Render settings (boundary smoothing, outlines and map colours), kept apart from the map config
    RenderSettings renderSettings;
    renderSettings.load_json(RENDER_SETTINGS_FILE);

	// Create the POP Manager
	PopManager popManager;

    // Create the view
    sf::Vector2f oldPos;
    bool moving = false;
    bool drawConvergenceLinesBool = false; // Set to true to draw the convergence lines of wind direction
    bool drawWindArrowsBool = false; // Set to true to draw the wind direction
    bool drawHighlightBool = false; // Set to true to highlight a cell
    bool drawRiversBool = true; // Draw rivers
	bool drawLakesBool = true; // Draw lakes
	bool drawContinentBool = false; // Draw continent borders

    int mapType = 0; int mapTypeOld = 0;
    bool showNewMapBool = false; // Get window to draw new map
    bool showBiomeGenBool = false; // Get window to regenerate biomes
	bool showResourceGenBool = false; // Get window to regenerate resources
	bool showPlanetaryParamsBool = false; // Show planetary parameters window
	bool showContinentViewerBool = false; // Show continent viewer
	bool showPopulationVierwerBool = false; // Show population viewer

	bool changeBiomeColorBool = false; // Change the color of the biomes

    // Save Load Configs
    bool showLoadConfig = false;
    bool showSaveConfig = false;
   
    // UI structs
    BiomeUI biomeUI;

    // Find window
	bool showFindSearcherBool = false;
	std::vector<std::size_t> findingCells;
	std::size_t findCell = vor::INVALID_INDEX;

    // Retrieve the window's default view
    float globalZoom = 1;
    sf::View view = window.getDefaultView();

    // Resource Map
	ResourceType selectedResource = ResourceType::Clay;

    // Seasonal day slider
    bool useSeasonalView = false;
    float currentDayOfYear = 0.0f;
    float previousDayOfYear = -1.0f;

	// Ticker simulation
    TickerSimulation ticker;
    float simulationSpeed = 1.0f;
	bool showSimulationBool = false; // Show simulation window

    sf::Clock deltaClock;

	// Seasonal Calculator (I should create a collected class for all of these calculators)
    SeasonalCalculator seasonCalc(globals);

    // Generate the actual map:
    genWorld(planet, globals, window,
        lines, seasonCalc,
        windowWidth, windowHeight,
		font, config, seed, popManager);
    window.setActive(true);
    planetView.rebuild(planet);

	std::size_t maxCellInMap = planet.cells.size();

    // Recolours the globe for the current map type, on the slider's day when the seasonal view is on
    auto redrawMap = [&]() {
        if (!window.setActive(true)) { return; }
        if (useSeasonalView) {
            planetView.setCellColors(planet, seasonalMapModeColors(planet, globals, renderSettings.colors, mapType, selectedResource, seasonCalc, currentDayOfYear));
        }
        else {
            planetView.setCellColors(planet, mapModeColors(planet.cells, globals, renderSettings.colors, mapType, selectedResource));
        }
    };
    redrawMap();

    while (window.isOpen())
    {
        if (showMainMenu)
        {
            mainMenu(window, config, windowWidth, windowHeight, showMainMenu);
        }

        sf::Event event;

        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(window, event);

            planetView.handleEvent(event, window);

            // The flat map's keys: Escape closes, T, B and P toggle the temperature, biome and precipitation maps
            if (event.type == sf::Event::KeyPressed && !showLoadConfig && !showSaveConfig && !ImGui::GetIO().WantCaptureKeyboard)
            {
                if (event.key.code == sf::Keyboard::Escape) { window.close(); }
                else if (event.key.code == sf::Keyboard::T) { mapType = mapType == 1 ? 0 : 1; }
                else if (event.key.code == sf::Keyboard::B) { mapType = mapType == 2 ? 0 : 2; }
                else if (event.key.code == sf::Keyboard::P) { mapType = mapType == 3 ? 0 : 3; }
            }

			if (event.type == sf::Event::Closed) {
				window.close();
			}

        }

        if (!window.isOpen()) {
            window.clear();

            ImGui::SFML::Shutdown(window);

            break;
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Map Controls");

        // Radio Buttons for the different maps
        ImGui::RadioButton("Height", &mapType, 0); ImGui::SameLine();
        ImGui::RadioButton("Temperature", &mapType, 1); ImGui::SameLine();
        ImGui::RadioButton("Biome", &mapType, 2); ImGui::SameLine();
        ImGui::RadioButton("Percepitation", &mapType, 3);
        ImGui::RadioButton("Continents", &mapType, 5); ImGui::SameLine();
		ImGui::RadioButton("Cultures", &mapType, 6); ImGui::SameLine();
		ImGui::RadioButton("Resources", &mapType, 7);
		ImGui::RadioButton("Humidity", &mapType, 4);
            
        if (mapType != mapTypeOld) {
            std::cout << "Map Type: " << mapType << std::endl;
			mapTypeOld = mapType;
            // Reset seasonal view when changing map types (except for temperature, precipitation, humidity)
            if (mapType != 1 && mapType != 3 && mapType != 4) {
                useSeasonalView = false;
            }
            redrawMap();
            if (mapType == 7) { showResourceGenBool = true; }
		}

        // Seasonal day slider (only visible when on Temperature, Precipitation, or Humidity map)
        if (mapType == 1 || mapType == 3 || mapType == 4) {
            ImGui::Separator();
            float yearLength = globals.planetaryParams.getYearLength();
            
            ImGui::Checkbox("Use Seasonal View", &useSeasonalView);
            if (ImGui::IsItemHovered()) {
                ImGui::Text("Enable to show climate at a specific day of year");
            }
            
            if (useSeasonalView) {
                ImGui::SliderFloat("Day of Year", &currentDayOfYear, 0.0f, yearLength, "%.0f");
                
                // Show the season at the centre of the view
                const char* seasonName = seasonCalc.getSeasonName(planetView.camera.lat, currentDayOfYear);
                ImGui::SameLine();
                ImGui::Text("(%s)", seasonName);
                
                // Update map when day changes
                if (std::abs(currentDayOfYear - previousDayOfYear) > 0.5f) {
                    redrawMap();
                    previousDayOfYear = currentDayOfYear;
                }
            } else {
                // If we just disabled seasonal view, redraw normal map
                if (previousDayOfYear >= 0.0f) {
                    redrawMap();
                    previousDayOfYear = -1.0f;
                }
            }
            ImGui::Separator();
        }

		// The overlays (convergence lines, wind arrows, rivers, lakes, plate borders) come back on the globe later
		ImGui::Checkbox("Highlight Cell", &drawHighlightBool);
		if (ImGui::BeginItemTooltip()) { ImGui::Text("Show the cell under the cursor in the Highlighted Cell window"); ImGui::EndTooltip(); }

		ImGui::Checkbox("Planetary Parameters", &showPlanetaryParamsBool);
		ImGui::Checkbox("Continent Viewer", &showContinentViewerBool);
		ImGui::Checkbox("Population Viewer", &showPopulationVierwerBool);

        ImGui::Text("Number of cells: %d", planet.cells.size());
        ImGui::Text("Number of biomes: %d", globals.biomes.size());
        ImGui::Text("Sealevel: %.2f", globals.seaLevel);
        ImGui::Text("Global Temperature: %.2f", globals.globalTempAvg);
        ImGui::Text("Global Precipitation: %.2f", globals.globalPercepitation);
        ImGui::Text("Global Snow Line: %.2f", globals.globalSnowline);
		ImGui::Text("Amount of Rivers: %d", globals.rivers.size());


        // Display the temp, percepitation, and elevation, biome of the highlighted cell at the same position.
        // The hovered cell stays while the mouse is over the UI, so the window can be used
        highlightedCell = drawHighlightBool && planetView.hoveredCell >= 0 ? static_cast<std::size_t>(planetView.hoveredCell) : vor::INVALID_INDEX;
        highligtedCellObservation(planet, globals, highlightedCell, &seasonCalc, &popManager);
        
        if (mapType==2)
        {
            biomeObservation(globals,changeBiomeColorBool);

            if (changeBiomeColorBool) {
                redrawMap();
                changeBiomeColorBool = false;
            }
        }

        ImGui::Checkbox("Draw New Map", &showNewMapBool);
        ImGui::Checkbox("Generate New Biomes", &showBiomeGenBool);
        ImGui::Checkbox("Simulation", &showSimulationBool);

		ImGui::Checkbox("Find Cell", &showFindSearcherBool); 
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Find a cell by its index"); ImGui::EndTooltip(); }

        if (ImGui::Button("Save Config", { 200,50 })) { showSaveConfig = !showSaveConfig; }
        else if (ImGui::Button("Load Config", { 200, 50 })) { showLoadConfig = !showLoadConfig; };

        // The boundary smoothing settings come back when the smoothing moves onto the sphere

        // Only the value maps use the map colours, redrawing the others would reroll the continent colours
        bool valueMap = mapType == 0 || mapType == 1 || mapType == 3;
        if (mapColorsUI(renderSettings.colors) && valueMap) { redrawMap(); }
        if (renderSettingsFileUI(renderSettings) && valueMap) { redrawMap(); }

        ImGui::End();

        planetView.ui(planet, globals, window);

        if (showSimulationBool) { tickerControls(ticker, simulationSpeed, planet, globals, config); }

		searchFinder(planet, findingCells, findCell, showFindSearcherBool, maxCellInMap);
        biomeUI.biomePopUp(planet, globals, config, showBiomeGenBool, mapType, changeBiomeColorBool);
        if (showNewMap(planet, globals, window,
			lines, seasonCalc,
                windowWidth, windowHeight,
                font, config, seed, popManager,
                showNewMapBool))
        {
            window.setActive(true);
            planetView.rebuild(planet);
            maxCellInMap = planet.cells.size();
            redrawMap();
        }
		configLoadSave(config, showLoadConfig, showSaveConfig);
		planetaryParamsViewer(globals, showPlanetaryParamsBool);
        continentViewer(globals, showContinentViewerBool);
		populationViewer(popManager, globals, showPopulationVierwerBool, planet);

        if (resourceMapController(globals, config, showResourceGenBool, mapType, selectedResource))
        {
            mapTypeOld = mapType;
            redrawMap();
        }

        planetView.update(window, planet);

        window.clear();
        planetView.draw(window);

        ImGui::SFML::Render(window);

        window.display();
        
    }
	ImPlot::DestroyContext();
    ImGui::SFML::Shutdown(window);

    // Stop the ticker simulation before exiting
    ticker.stop();

    return 0;
}

