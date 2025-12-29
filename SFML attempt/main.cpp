#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <cstdlib>
#include <chrono>

#include "mapconfig.hpp"
#include "windArrows.hpp"
#include "Voronoi.hpp"
#include "vertex.hpp"
#include "cell.hpp"
#include "GlobalWorldObjects.hpp"
#include "Map.hpp"
#include "POP.hpp"
#include "SeasonalCalculator.hpp"

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
	bool showResourceGenBool = false; // Get window to regenerate resources
	bool showPlanetaryParamsBool = false; // Show planetary parameters window
	bool showContinentViewerBool = false; // Show continent viewer

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
    SeasonalCalculator seasonCalc(globals, windowHeight / 2);

    // Generate the actual map:
    genWorld(map, globals, window, vertexMap,
        lines, seasonCalc,
        windowWidth, windowHeight,
		font, config, seed);

    // Draw initial wind arrows
	windArrows = generateWindArrows(map, globalZoom, view, window.getSize());

	std::size_t maxCellInMap = map.cells.size();

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
				windowWidth, windowHeight, showLoadConfig, showSaveConfig,
                windArrows, drawWindArrowsBool
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
            switch (mapType) {
			case 0:
				drawHeightMap(map, vertexMap);
                drawWindArrowsBool = false;
				break;
			case 1:
                if (useSeasonalView) {
                    drawSeasonalTempMap(map, vertexMap, seasonCalc, currentDayOfYear);
                } else {
				    drawTempMap(map, vertexMap);
                }
                drawWindArrowsBool = false;
				break;
			case 2:
				drawBiomeMap(map, globals, vertexMap);
                drawWindArrowsBool = false;
				break;
			case 3:
                if (useSeasonalView) {
                    drawSeasonalPercepitationMap(map, vertexMap, seasonCalc, currentDayOfYear);
                } else {
				    drawPercepitationMap(map, vertexMap);
                }
                drawWindArrowsBool = false;
				break;
            case 5:
                drawContinentMap(map, globals, vertexMap);
                drawWindArrowsBool = false;
                break;
			case 6:
                drawCulturesMap(map, globals, vertexMap);
                drawWindArrowsBool = false;
				break;
			case 7:
                drawResourceMap(map, vertexMap, selectedResource);
				showResourceGenBool = true;
				drawWindArrowsBool = false;
                break;
            case 4:
                if (useSeasonalView) {
                    drawSeasonalHumidityMap(map, vertexMap, seasonCalc, currentDayOfYear);
                } else {
				    drawHumidityMap(map, vertexMap);
                }
				drawWindArrowsBool = false;
                break;
			}
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
                
                // Show season name
                sf::Vector2f centerPos(windowWidth / 2.0f, windowHeight / 2.0f);
                const char* seasonName = seasonCalc.getSeasonName(centerPos, currentDayOfYear);
                ImGui::SameLine();
                ImGui::Text("(%s)", seasonName);
                
                // Update map when day changes
                if (std::abs(currentDayOfYear - previousDayOfYear) > 0.5f) {
                    if (mapType == 1) {
                        drawSeasonalTempMap(map, vertexMap, seasonCalc, currentDayOfYear);
                    } else if (mapType == 3) {
                        drawSeasonalPercepitationMap(map, vertexMap, seasonCalc, currentDayOfYear);
                    } else if (mapType == 4) {
                        drawSeasonalHumidityMap(map, vertexMap, seasonCalc, currentDayOfYear);
                    }
                    previousDayOfYear = currentDayOfYear;
                }
            } else {
                // If we just disabled seasonal view, redraw normal map
                if (previousDayOfYear >= 0.0f) {
                    if (mapType == 1) {
                        drawTempMap(map, vertexMap);
                    } else if (mapType == 3) {
                        drawPercepitationMap(map, vertexMap);
                    } else if (mapType == 4) {
                        drawHumidityMap(map, vertexMap);
                    }
                    previousDayOfYear = -1.0f;
                }
            }
            ImGui::Separator();
        }

		ImGui::Checkbox("Draw Lines", &drawConvergenceLinesBool); ImGui::SameLine();
        ImGui::Checkbox("Wind Arrows", &drawWindArrowsBool);
		ImGui::Checkbox("Highlight Cell", &drawHighlightBool); ImGui::SameLine();
		ImGui::Checkbox("Draw Rivers", &drawRiversBool); 
		ImGui::Checkbox("Draw Lakes", &drawLakesBool); 
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Drawing lakes is currently broken."); ImGui::EndTooltip(); }

		ImGui::Checkbox("Planetary Parameters", &showPlanetaryParamsBool);
		ImGui::Checkbox("Continent Viewer", &showContinentViewerBool);

        ImGui::Text("Number of cells: %d", map.cells.size());
        ImGui::Text("Number of biomes: %d", globals.biomes.size());
        ImGui::Text("Sealevel: %.2f", globals.seaLevel);
        ImGui::Text("Global Temperature: %.2f", globals.globalTempAvg);
        ImGui::Text("Global Precipitation: %.2f", globals.globalPercepitation);
        ImGui::Text("Global Snow Line: %.2f", globals.globalSnowline);
		ImGui::Text("Amount of Rivers: %d", globals.rivers.size());


        // Display the temp, percepitation, and elevation, biome of the highlighted cell at the same position
        highligtedCellObservation(map, globals, highlightedCell, &seasonCalc);
        
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
        ImGui::Checkbox("Simulation", &showSimulationBool);

		ImGui::Checkbox("Find Cell", &showFindSearcherBool); 
        if (ImGui::BeginItemTooltip()) { ImGui::Text("Find a cell by its index"); ImGui::EndTooltip(); }

        if(ImGui::Button("Switch origin of vertexMap")) { vertexMap.switchOrigin(map); };
        if (ImGui::Button("Check vertexMap")) { std::cout << vertexMap.useVertexBuffer << " : Array: " << vertexMap.vertexArray.getVertexCount() << " : Buffer: " << vertexMap.vertexBuffer.getVertexCount() << std::endl; };

        if (ImGui::Button("Save Config", { 200,50 })) { showSaveConfig = !showSaveConfig; }
        else if (ImGui::Button("Load Config", { 200, 50 })) { showLoadConfig = !showLoadConfig; };

        ImGui::End();

        if (showSimulationBool) { tickerControls(ticker, simulationSpeed, map, globals, config, vertexMap); }

		searchFinder(map, findingCells, findCell, showFindSearcherBool, maxCellInMap);
        biomeUI.biomePopUp(map, globals, config, showBiomeGenBool, mapType, changeBiomeColorBool);
        showNewMap(map, globals, window, vertexMap,
			windArrows, lines, seasonCalc,
                windowWidth, windowHeight,
                font, config, seed, 
                showNewMapBool);
		configLoadSave(config, showLoadConfig, showSaveConfig);
		planetaryParamsViewer(globals, showPlanetaryParamsBool);
        continentViewer(globals, showContinentViewerBool);

        if (showResourceGenBool)
        {
			resourceMapController(map, globals, vertexMap, config, showResourceGenBool, mapType, selectedResource);
        }

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

