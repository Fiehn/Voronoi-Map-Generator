#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <cstdlib>
#include <chrono>

#include "mapconfig.hpp"
#include "Voronoi.hpp"
#include "vertex.hpp"
#include "cell.hpp"
#include "GlobalWorldObjects.hpp"
#include "Map.hpp"

#include "imgui.h"
#include "imgui-SFML.h"
#include "Include/ImGuiFD-main/ImGuiFD.h"
#include "Include/ImPlot/implot.h"

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

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "SFML");
    window.setFramerateLimit(27); // For now there is no reason to have even this high framerate
    ImGui::SFML::Init(window);
	ImPlot::CreateContext();

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

    // Create empty highlighted cell
    std::size_t highlightedCell = vor::INVALID_INDEX;
    sf::VertexArray highlight(sf::LineStrip, 5);
    
    // Load font!
    sf::Font font;
    if (!font.loadFromFile("Fonts/Roboto-Medium.ttf")) { std::cout << "Could not load font" << std::endl; }

    // Init the map
    vor::Voronoi map;
    
    // Create the vertex map
    VertexMap vertexMap;
    std::cout << "Vertex Buffer Available? " << vertexMap.useVertexBuffer << std::endl;

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

	// Ticker simulation
    TickerSimulation ticker;
    float simulationSpeed = 1.0f;

    sf::Clock deltaClock;

    // Generate the actual map:
    genWorld(map, globals, window, vertexMap,
        windArrows, lines, 
        windowWidth, windowHeight,
		font, config, seed);

	std::size_t maxCellInMap = map.cells.size();

	ticker.start(map, globals, config, vertexMap); // Start the ticker simulation

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

			eventloop(window, event, 
				mapType, moving, oldPos,
				drawHighlightBool, highlight,
				map, highlightedCell, view, globalZoom,
				windowWidth, windowHeight, showLoadConfig, showSaveConfig
                );

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
        ImGui::RadioButton("Wind", &mapType, 4);
            
        if (mapType != mapTypeOld) {
            std::cout << "Map Type: " << mapType << std::endl;
			mapTypeOld = mapType;
            switch (mapType) {
			case 0:
				drawHeightMap(map, vertexMap);
                drawWindArrowsBool = false;
				break;
			case 1:
				drawTempMap(map, vertexMap);
                drawWindArrowsBool = false;
				break;
			case 2:
				drawBiomeMap(map, globals, vertexMap);
                drawWindArrowsBool = false;
				break;
			case 3:
				drawPercepitationMap(map, vertexMap);
                drawWindArrowsBool = false;
				break;
            case 4:
				drawWindMap(map, vertexMap);
                drawWindArrowsBool = true;
				break;
            case 5:
                drawContinentMap(map, globals, vertexMap);
                drawWindArrowsBool = false;
                break;
			}
		}

        ImGui::Checkbox("Draw Lines", &drawConvergenceLinesBool);
        ImGui::Checkbox("Wind Arrows", &drawWindArrowsBool);
        ImGui::Checkbox("Highlight Cell", &drawHighlightBool); 
		ImGui::Checkbox("Draw Rivers", &drawRiversBool); 
		ImGui::Checkbox("Draw Lakes", &drawLakesBool); 
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Drawing lakes is currently broken."); ImGui::EndTooltip(); }


        ImGui::Text("Number of cells: %d", map.cells.size());
        ImGui::Text("Number of biomes: %d", globals.biomes.size());
        ImGui::Text("Sealevel: %.2f", globals.seaLevel);
        ImGui::Text("Global Temperature: %.2f", globals.globalTempAvg);
        ImGui::Text("Global Precipitation: %.2f", globals.globalPercepitation);
        ImGui::Text("Global Snow Line: %.2f", globals.globalSnowline);
		ImGui::Text("Amount of Rivers: %d", globals.rivers.size());


        // Display the temp, percepitation, and elevation, biome of the highlighted cell at the same position
        highligtedCellObservation(map, globals, highlightedCell);
        
        if (mapType==2)
        {
            biomeObservation(globals,changeBiomeColorBool);

            if (changeBiomeColorBool) {
                drawBiomeMap(map, globals, vertexMap);
                changeBiomeColorBool = false;
            }
        }

        ImGui::Checkbox("Draw New Map", &showNewMapBool);
        ImGui::Checkbox("Generate New Biomes", &showBiomeGenBool);

		ImGui::Checkbox("Find Cell", &showFindSearcherBool); 
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Find a cell by its index"); ImGui::EndTooltip(); }

        if(ImGui::Button("Switch origin of vertexMap")) { vertexMap.switchOrigin(map); };
        if (ImGui::Button("Check vertexMap")) { std::cout << vertexMap.useVertexBuffer << " : Array: " << vertexMap.vertexArray.getVertexCount() << " : Buffer: " << vertexMap.vertexBuffer.getVertexCount() << std::endl; };

        if (ImGui::Button("Save Config", { 200,50 })) { showSaveConfig = !showSaveConfig; }
        else if (ImGui::Button("Load Config", { 200, 50 })) { showLoadConfig = !showLoadConfig; };

        ImGui::End();

        //// TICKER CONTRLS
        ImGui::Begin("Simulation Controls");
        ImGui::SliderFloat("Simulation Speed", &simulationSpeed, 0.1f, 10.0f);
        if (ImGui::Button)
        if (ImGui::Button("Pause")) {
            ticker.pause();
        }
        if (ImGui::Button("Resume")) {
            ticker.resume();
        }
        if (ImGui::Button("Stop")) {
            ticker.stop();
        }
        ticker.setSpeed(simulationSpeed);

        ImGui::End();

		searchFinder(map, findingCells, findCell, showFindSearcherBool, maxCellInMap);
        biomeUI.biomePopUp(map, globals, config, showBiomeGenBool, mapType, changeBiomeColorBool);
        showNewMap(map, globals, window, vertexMap,
                windArrows, lines,
                windowWidth, windowHeight,
                font, config, seed, 
                showNewMapBool);
		configLoadSave(config, showLoadConfig, showSaveConfig);


        if (!drawHighlightBool) {
            highlight.clear();
            highlightedCell = vor::INVALID_INDEX;
        }

        window.clear();
        
        vertexMap.draw(window);
        
        if (drawConvergenceLinesBool) {
			window.draw(lines);
		}
        if (drawWindArrowsBool) {
			window.draw(windArrows);
		}
        if (highlightedCell != vor::INVALID_INDEX) {
			window.draw(highlight);
		}
        if (showFindSearcherBool)
        {
            sf::VertexArray new_highlight;
            for (int i = 0; i < findingCells.size(); i++)
            {
                if (findingCells[i] < map.cells.size())
                {
                    new_highlight = drawHighlightCell(map, findingCells[i]);
                    window.draw(new_highlight);
                }
            }
        }
        if (drawRiversBool)
		{
			drawRivers(globals, window);
		}
        if (drawLakesBool)
        {
			drawLakes(globals, window);
        }
        if (drawContinentBool)
        {
            drawContinents(globals, window);
        }

        /// CHANGE THIS TO A BOOL!
        if (mapType == 5)
        {
			for (int i = 0; i < globals.continents.size(); i++)
            {
				window.draw(globals.continents[i].drawDirectionArrows()); 
			}
        }

        ImGui::SFML::Render(window);

        window.display();
        
    }
	ImPlot::DestroyContext();
    ImGui::SFML::Shutdown(window);

    // Stop the ticker simulation before exiting
    ticker.stop();

    return 0;
}

