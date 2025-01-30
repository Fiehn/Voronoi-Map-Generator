#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <cstdlib>
#include <chrono>
#include "Voronoi.hpp"
#include "vertex.hpp"
#include "GlobalWorldObjects.hpp"
#include "cell.hpp"
#include "Map.hpp"
#include "mapconfig.hpp"

#include "imgui.h"
#include "imgui-SFML.h"
#include "Include/ImGuiFD-main/ImGuiFD.h"



static void loadText(sf::RenderWindow& window, sf::Text& text, int fontsize, std::string& displayText, std::string newText)
{
    displayText = displayText.substr(0, displayText.length() - 3) + "(X)" + "\n" + newText + " (0)";
    text.setString(displayText);
    text.move(0, - fontsize);
    window.clear();
    window.draw(text);
    window.display();
}

static void genWorld(vor::Voronoi& map, 
    GlobalWorldObjects& globals, 
    sf::RenderWindow& window, 
    VertexMap& vertexMap,
    sf::VertexArray& windArrows,
    sf::VertexArray& lines,
    const unsigned int MAXWIDTH,
	const unsigned int MAXHEIGHT,
    const sf::Font& font,
	MapConfig& config,
    unsigned int seed
) {
    // Seed
    if (seed == 0) { seed = time(NULL); }
    std::srand(seed);

    // Globals
    globals.clearGlobals();
    globals.setSeaLevel(config.sealevel); // RandomBetween(0.4f, 0.6f)
    globals.setGlobalTemp(config.global_temp_avg);
    globals.generateConvergenceLines(config.n_convergence_lines, config.windstr_alpha, config.windstr_beta);
    lines.clear();
    lines.resize(2 * config.n_convergence_lines);
    for (int i = 0; i < config.n_convergence_lines * 2; i++)
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


    // Initialize the loading screen
    std::string loadingText = "Initializing (0)";
    sf::Text text(loadingText, font, 50);
    text.setFillColor(sf::Color::White);
    text.setPosition(MAXWIDTH / 2 - text.getGlobalBounds().width / 2, MAXHEIGHT / 2 - text.getGlobalBounds().height / 2);
    // Draw the loading screen
    window.clear();
    window.draw(text);
    window.display();

    // Create the map
    auto start = std::chrono::high_resolution_clock::now();
    map.clearMap();
    map.fillMap(config.ncellx, config.ncelly, MAXWIDTH, MAXHEIGHT, config.point_jitter);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Point Map took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Generating Heightmap");
    random_height_gen(map.cells, config.npeaks, config.delta_max_neg, config.delta_max_pos, config.prob_of_island, config.dist_from_mainland, config.height_method);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Height Gen took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Smoothing Heightmap");
    smooth_height(map.cells, config.rise_threshold, config.height_smooth_repeats, config.smooth_method);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Smooth Height took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Adding Noise to Heightmap");
    noise_height(map.cells, config.height_noise_repeats);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Noise Height took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Calculating Height Values");
    calcHeightValues(map.cells, globals, config.delta_coast_line);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Height Values took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Distance To Oceans");
    closeOceanCell(map.cells, globals);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Dist to Ocean took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Calculating Wind");
    calcWind(map.cells, map.points, MAXHEIGHT, globals);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Wind Calc took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Calculating River");
    calcRiverStart(map.cells, globals, map.points, map.voronoi_points);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Rivers took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Calculating Temperatures");
    calcTemp(map.cells, globals, map.points, MAXHEIGHT);
    smoothTemps(map.cells, config.temp_smooth_repeats);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Temperature took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Calculating Percepetation");
    calcPercepitation(map.cells, map.points, globals, config.percepitation_repeats);
    smoothPercepitation(map.cells, config.percepitation_smooth_repeats);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Percepitatiton took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Calculating Humidity");
    calcHumid(map.cells);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Humidity took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Calculating Biomes");
    calcBiome(map.cells, globals, config.kmeans_max_iter, config.biome_method);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Biomes took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Drawing Wind Arrows");
    windArrows.clear();
    windArrows = vor::windArrows(map);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Wind took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    loadText(window, text, 50, loadingText, "Generating Vertex Buffer");
    vertexMap.clear();
    vertexMap.create(map);
    vertexMap.genVertexMap(map);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Vertex Map took: " << duration.count() << "ms" << std::endl;

    loadText(window, text ,50, loadingText,"Drawing Map");

    return;
}

static sf::VertexArray drawHighlightCell(vor::Voronoi& map, std::size_t cell)
{
    sf::VertexArray highlight(sf::LinesStrip, map.cells[cell].vertex.size() + 1);
    for (size_t i = 0; i < map.cells[cell].vertex.size(); i++) {
		highlight[i].position = sf::Vector2f(map.voronoi_points[map.cells[cell].vertex[i]].x, map.voronoi_points[map.cells[cell].vertex[i]].y);
		highlight[i].color = sf::Color::Red;
	}
    highlight[map.cells[cell].vertex.size()].position = sf::Vector2f(map.voronoi_points[map.cells[cell].vertex[0]].x, map.voronoi_points[map.cells[cell].vertex[0]].y);
    highlight[map.cells[cell].vertex.size()].color = sf::Color::Red;
    return highlight;
}

static void drawTempMap(vor::Voronoi& map, VertexMap& vertexMap) {
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(255, 255 / 2 + clamp(5 * map.cells[i].temp, 255 / 2, -255), 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawBiomeMap(vor::Voronoi& map, GlobalWorldObjects globals, VertexMap& vertexMap)
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = globals.biomes[map.cells[i].biome].color;
        }
    }
    vertexMap.update(map);
}

static void drawPercepitationMap(vor::Voronoi& map, VertexMap& vertexMap)
{
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(0, clamp(5 * map.cells[i].percepitation, 255, 0), 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawHeightMap(vor::Voronoi& map, VertexMap& vertexMap)
{
    for (std::size_t i = 0; i < map.cells.size(); i++) {
        sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + 1.75), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawWindMap(vor::Voronoi& map, VertexMap& vertexMap) {
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(255 * map.cells[i].windDir / 360, 255 * map.cells[i].windStr, 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    vertexMap.update(map);
}

static void drawRivers(GlobalWorldObjects& globals, sf::RenderWindow& window)
{
	for (size_t i = 0; i < globals.rivers.size(); i++)
	{
		sf::VertexArray river = globals.rivers[i].drawRiver();
		window.draw(river);
	}
}

static void drawLakes(GlobalWorldObjects& globals, sf::RenderWindow& window)
{
    for (std::size_t i = 0; i < globals.lakes.size(); i++)
    {
		/// TODO: Fix the lake drawing
        //sf::VertexArray lake = globals.lakes[i].drawLake();
		//window.draw(lake);
	}
}

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
    if (!font.loadFromFile("Roboto-Medium.ttf")) { std::cout << "Could not load font" << std::endl; }

    // Init the map
    vor::Voronoi map;
    
    // Create the vertex map
    VertexMap vertexMap;
    std::cout << "Vertex Buffer Available? " << vertexMap.useVertexBuffer << std::endl;

	// Create the config
    MapConfig config;

    // Create the view
    sf::Vector2f oldPos;
    bool moving = false;
    bool drawConvergenceLinesBool = false; // Set to true to draw the convergence lines of wind direction
    bool drawWindArrowsBool = false; // Set to true to draw the wind direction
    bool drawHighlightBool = false; // Set to true to highlight a cell
    bool drawRiversBool = true; // Draw rivers
	bool drawLakesBool = true; // Draw lakes

    int mapType = 0; int mapTypeOld = 0;
    bool showNewMapBool = false; // Get window to draw new map
    bool showBiomeGenBool = false; // Get window to regenerate biomes

    // Save Load Configs
    bool showLoadConfig = false;
    bool showSaveConfig = false;

    // Find window
	bool showFindSearcherBool = false;
	std::vector<std::size_t> findingCells;
	std::size_t findCell = vor::INVALID_INDEX;

    // Retrieve the window's default view
    float globalZoom = 1;
    sf::View view = window.getDefaultView();

    sf::Clock deltaClock;

    // Generate the actual map:
    genWorld(map, globals, window, vertexMap,
        windArrows, lines, 
        windowWidth, windowHeight,
		font, config, seed);

	std::size_t maxCellInMap = map.cells.size();
    
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(window, event);

            switch (event.type) {
            case sf::Event::Closed:
            {
                window.close();
                break;
            }

            // Mouse buttons
            case sf::Event::MouseButtonPressed:
                // Mouse button is pressed, get the position and set moving as active
                if (event.mouseButton.button == 0) {
                    moving = true;
                    oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                }
                if (event.mouseButton.button == 1) {
                    // This will need to be a values changer.
                }
                break;

            case sf::Event::MouseButtonReleased:

                // Mouse button is released, no longer move
                if (event.mouseButton.button == 0) {
                    moving = false;
                }
                break;

            case sf::Event::KeyPressed:

                // Close Program
                if (event.key.code == sf::Keyboard::Escape) { window.close(); break; }

                // Generate a new Map
                else if (event.key.code == sf::Keyboard::N)
                {
                    genWorld(map, globals, window, vertexMap,
                        windArrows, lines,
                        windowWidth, windowHeight,
                        font, config, seed);
                }

                // Draw Lines
                else if (event.key.code == sf::Keyboard::L) { drawConvergenceLinesBool = !drawConvergenceLinesBool; }

                // Wind Map
                else if (event.key.code == sf::Keyboard::W) {
                    if (mapType == 4) { mapType = 0; }
                    else { mapType = 4; }
                }

                // Temperature map
                else if (event.key.code == sf::Keyboard::T) {
                    if (mapType == 1) { mapType = 0; }
                    else { mapType = 1; }
                }

                // Biome Map
                else if (event.key.code == sf::Keyboard::B) {
                    if (mapType == 2) { mapType = 0; }
                    else { mapType = 2; } 
                }

                // Percepitation map
                else if (event.key.code == sf::Keyboard::P) {
                    if (mapType == 3) { mapType = 0; }
                    else { mapType = 3; }
                }
                
                // activate arrows for wind direction
                else if (event.key.code == sf::Keyboard::Comma) { drawWindArrowsBool = !drawWindArrowsBool; }

                // Activate Cell Highlighting
                else if (event.key.code == sf::Keyboard::H) { drawHighlightBool = !drawHighlightBool; highlight.clear(); }

				break;

            // Mouse Movement
            case sf::Event::MouseMoved:
            {
                // If no button is down, we are not moving the view
                if (!moving) {
                    // Highlight Cells if active 
                    if (drawHighlightBool)
                    {
                        // Get the position of the mouse in window coordinates
                        const sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
                        // Convert the position to world coordinates
                        const sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);
                        // Find the cell that contains the mouse
                        const std::size_t cellIndex = map.getCellIndex(worldPos);
                        // If the mouse is over a cell, highlight it
                        if (cellIndex != vor::INVALID_INDEX)
                        {
                            // Highlight the cell
                            highlightedCell = cellIndex;
                            highlight = drawHighlightCell(map, highlightedCell);
                        }
                        else
                        {
                            highlightedCell = vor::INVALID_INDEX;
                            highlight.clear();
                        }
                    }
                    break;
                }

                // Determine the new position in world coordinates
                const sf::Vector2f newPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
                // Determine how the cursor has moved
                // Swap these to invert the movement direction
                const sf::Vector2f deltaPos = oldPos - newPos;

                sf::Vector2f view_center = view.getCenter() + deltaPos;

                // Make sure that the view is contained inside the map
                if ((view.getCenter().x + deltaPos.x) + globalZoom * windowWidth / 2 > windowWidth || (view.getCenter().x + deltaPos.x) - globalZoom * windowWidth / 2 < 0) {
                    view_center.x = view.getCenter().x;
                }
                if ((view.getCenter().y + deltaPos.y) + globalZoom * windowHeight / 2 > windowHeight || (view.getCenter().y + deltaPos.y) - globalZoom * windowHeight / 2 < 0) {
                    view_center.y = view.getCenter().y;
                }

                view.setCenter(view_center);
                window.setView(view);

                // Save the new position as the old one
                oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
                break;
            }

            // Zoom
            case sf::Event::MouseWheelScrolled: {
                // Ignore the mouse wheel unless we're not moving
                if (moving) { break; }

                if (event.mouseWheelScroll.delta <= -1)
                {
                    globalZoom = std::min(1.f, globalZoom + .07f);
                }
                else if (event.mouseWheelScroll.delta >= 1)
                {
                    globalZoom = std::max(.1f, globalZoom - .07f);
                }

                // Update our view
                view.setSize(window.getDefaultView().getSize());
                view.zoom(globalZoom);

                // Make sure that the view is contained inside the map
                sf::Vector2f view_center = view.getCenter();
                if ((view.getCenter().x) + globalZoom * windowWidth / 2 > windowWidth) {
                    view_center.x = windowWidth - globalZoom * windowWidth / 2;
                }
                if ((view.getCenter().x) - globalZoom * windowWidth / 2 < 0) {
                    view_center.x = globalZoom * windowWidth / 2;
                }
                if ((view.getCenter().y) + globalZoom * windowHeight / 2 > windowHeight) {
                    view_center.y = windowHeight - globalZoom * windowHeight / 2;
                }
                if (view.getCenter().y - globalZoom * windowHeight / 2 < 0) {
                    view_center.y = globalZoom * windowHeight / 2;
                }
                view.setCenter(view_center);

                window.setView(view);
                break;
            }
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
        if (highlightedCell != vor::INVALID_INDEX) {
			const Cell& cell = map.cells[highlightedCell];
			ImGui::Text("Cell %d", highlightedCell);
			ImGui::Text("Temp: %.2f", cell.temp);
            ImGui::Text("Precipitation: %.2f", cell.percepitation);
            ImGui::Text("Elevation: %.2f", cell.height); ImGui::SameLine();
            ImGui::Text("Rise: %.2f", cell.rise);
            ImGui::Text("Distance to Ocean: %d", cell.distToOcean);
			ImGui::Text("Coast: %.d", cell.coastBool); ImGui::SameLine();
			ImGui::Text("Ocean: %.d", cell.oceanBool); ImGui::SameLine();
			ImGui::Text("River: %.d", cell.riverBool); ImGui::SameLine();
			ImGui::Text("Lake: %.d", cell.lakeBool);

            if (cell.riverBool)
            {
				ImGui::Text("River id: %d", cell.riverId);
			}
			if (cell.lakeBool)
			{
				ImGui::Text("Lake id: %d", cell.lakeId);
            }

            const Biome& biome = globals.biomes[cell.biome];
            ImVec4 color = ImVec4(biome.color.r / 255.0f, biome.color.g / 255.0f, biome.color.b / 255.0f, 1.0f);
            ImGui::Text("Biome: %s", biome.name.c_str());
            ImGui::SameLine();
            ImGui::ColorEdit4("", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip);
            if (mapType == 2)
            {
                ImGui::Text("Biome Probabilities: ");
                for (int i = 0; i < cell.biome_prob.size(); i++)
                {
					const Biome& biome = globals.biomes[i];
					ImGui::Text("%s: %.2f", biome.name.c_str(), cell.biome_prob[i]);
				}
            }
            ImGui::Text("Wind: %.2f, %.2f", cell.windDir, cell.windStr);
		}
        
        if (mapType==2)
        {
            bool change = true; // If the user changes the color of a biome, we need to update the map

            ImGui::Begin("Biome Generation Controls");
            for (int i = 0; i < globals.biomes.size(); i++)
            {
                Biome& biome = globals.biomes[i];
                float color[4];
                color[0] = biome.color.r / 255.0f;
                color[1] = biome.color.g / 255.0f;
                color[2] = biome.color.b / 255.0f;
                color[3] = 1.0f;
                ImGui::PushID(i);
                ImGui::Text("%s", biome.name.c_str());
                ImGui::SameLine();
                ImGui::ColorEdit4("", color, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
                ImGui::NewLine();

                int totalLength = 0;
                for (const auto& pair : biome.values) {
                    float intPart;
                    float fractPart = std::modf(pair.second, &intPart);

                    ImGui::SameLine();
                    if (fractPart == 0.0)
                    {
                        ImGui::Text("%s: %.0f", pair.first.c_str(), pair.second);
                    }
                    else
                    {
						ImGui::Text("%s: %.2f", pair.first.c_str(), pair.second);
					}
					totalLength += pair.first.length() + 5;
                    if (totalLength > 60) {
						totalLength = 0;
						ImGui::NewLine();
                    }
                }
                ImGui::Text("Size: %d", biome.numCells);
                
                ImGui::PopID(); // HERE MAP

                if (color[0] != biome.color.r / 255.0f || color[1] != biome.color.g / 255.0f || color[2] != biome.color.b / 255.0f) {
					change = false;
                    biome.color.r = color[0] * 255;
                    biome.color.g = color[1] * 255;
                    biome.color.b = color[2] * 255;
				}
            }

            if (!change) {
				drawBiomeMap(map, globals, vertexMap);
                change = true;
			}
            ImGui::End();
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

        if (showFindSearcherBool)
        {
			ImGui::Begin("Find Cell");

            unsigned int findCellUInt = findCell;
			ImGui::InputUInt("Cell", &findCellUInt);
			findCell = findCellUInt;
			if (ImGui::Button("Find Cell")) {
				if (findCell < map.cells.size()) {
                    // if the cell is found and in the list
                    if (std::find(findingCells.begin(), findingCells.end(), findCell) == findingCells.end() && findCell <= maxCellInMap)
                    {
                        findingCells.push_back(findCell);
                    }
				}
			}
            for (int i = 0; i < findingCells.size(); i++)
            {
				ImGui::PushID(i);
				ImGui::Text("Cell %d", findingCells[i]); ImGui::SameLine();
                if (ImGui::Button("Remove")) {
					findingCells.erase(findingCells.begin() + i);
                }
				ImGui::PopID();
            }
            
			ImGui::End();
        }

        if (showBiomeGenBool)
        {
            ImGui::Begin("Biome Generation Controls");

            if (ImGui::Button("Generate Biomes", { 200,50 })) {
                globals.biomes.clear();
                std::vector<sf::Color> biomeColors = randomColors(config.n_biomes);
                for (int i = 0; i < config.n_biomes; i++) {
                    globals.addBiome("Biome" + std::to_string(i), biomeColors[i]);
                }
                biomeColors.clear();
				calcBiome(map.cells, globals, config.kmeans_max_iter, config.biome_method, config.prob_smoothing);
			}
            ImGui::PushItemWidth(150.f);
            ImGui::InputUInt("KMeans Max Iterations", &config.kmeans_max_iter);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount of maximum iterations for clustering. Keep above 10");
                ImGui::EndTooltip();
            }
			ImGui::SliderFloat("Probability Smoothing", &config.prob_smoothing, 0.0f, 5.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Factor of weight neighbors have on the probability of biome for each cell");
                ImGui::EndTooltip();
            }
			ImGui::InputUInt("Method", &config.biome_method);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Method of biome generation. 1: GMM with smoothing, 2: K-means, 3: GMM no smoothing");
                ImGui::EndTooltip();
            }
            ImGui::InputUInt("Amount of Biomes", &config.n_biomes);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount of biomes to generate, if all land becomes one biome generate more.");
                ImGui::EndTooltip();
            }
            ImGui::End();
        }

        if (showNewMapBool)
        {
            ImGui::Begin("New Map Controls");
            // push a color on the generate new map button

            if (ImGui::Button("Generate New Map", {200,50})) {
                genWorld(map, globals, window, vertexMap,
                    windArrows, lines,
                    windowWidth, windowHeight,
                    font, config, seed);
                showNewMapBool = false;
            }

            ImGui::Text("The map will have around %.d cells", config.ncellx * config.ncelly);

            ImGui::PushItemWidth(150.f);

            ImGui::InputUInt("Cells x", &config.ncellx);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Number of cells in the x direction. \nKeep at least above 100 or it starts to break down.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Cells y", &config.ncelly);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Number of cells in the y direction. \nKeep at least above 100 or it starts to break down.");
                ImGui::EndTooltip(); }

            ImGui::DragFloat("Jitter of the cells", &config.point_jitter,0.5f,0.0f,10.f);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount each cell is jittered from their grid position");
                ImGui::EndTooltip(); }

            ImGui::DragFloat("Sea Level", &config.sealevel, 0.01f, 0.0f, 1.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("The height of the sealevel across map, between 0 and 1.");
                ImGui::EndTooltip(); }

            ImGui::DragFloat("Global Temp Avg", &config.global_temp_avg, 0.5f, -30.0f, 100.0f);
            if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("This is an initializer for global temperature modifier, \naround 30-50 will give realistic values (to earth) depending on the size of the map.");
				ImGui::EndTooltip(); }

            ImGui::InputUInt("Number of Peaks", &config.npeaks);
            if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("Number of peaks to generate on the map.");
				ImGui::EndTooltip(); }

            ImGui::InputUInt("Nr of Convergence Lines", &config.n_convergence_lines);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Defines lines that split the prevailing winds. \nDirections and strengths are then concluded randomly, Earth has 6 zones so input 6.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Amount of Biomes", &config.n_biomes);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount of biomes to generate initially. \nLarge maps will often have a lot of ocean biomes.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Height Smooths", &config.height_smooth_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount of times the heihgt map is smoothed. \nShould be above 2 ideally more. \nThis is relatively intensive.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Height Noisers", &config.height_noise_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("This is the amount of times the height values are renoised after the smoothing, \nthis gives a more realistic height map. \nShould not be used more times than smoothing.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Temp Smooths", &config.temp_smooth_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("This is the amount of times temperature is smoothed out.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Percepitations", &config.percepitation_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("This is the amount of times percepitation is calculated. \nShould not be more than 1 unless you want high contrast.");
                ImGui::EndTooltip(); }
            if (config.percepitation_repeats == 0) { config.percepitation_repeats = 1; }

            ImGui::InputUInt("Percepitation Smooths", &config.percepitation_smooth_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("This is the amount of times percepitation is smoothed.");
                ImGui::EndTooltip(); }
            
            //ImGui::Checkbox("Advanced Settings", &advancedSettings);

            if (ImGui::CollapsingHeader("Advanced Settings")) {
                ImGui::InputUInt("Seed", &seed);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Seed for the random number generator. \nKeep at 0 for random seed.");
                    ImGui::EndTooltip();
                }
                ImGui::DragFloat("Delta Max Neg", &config.delta_max_neg, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The maximum amount of random height added in the negative direction. \nIncreasing will make the map more steepely falling");
                    ImGui::EndTooltip();
                }
                ImGui::DragFloat("Delta Max Pos", &config.delta_max_pos, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The maximum amount of random height added in the positive direction. \nIncreasing will make the map more steepely falling");
                    ImGui::EndTooltip();}
                ImGui::DragFloat("Islands", &config.prob_of_island, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("small probability of random height increase when away from mainland. \nIncrease to make more islands at a short distance from shore.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Distance from land", &config.dist_from_mainland, 0.04f, 0.0f, 5.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The distance from the mainland where the probability of random height increase begins,\n Represented by the sum of height of all neighbors");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Rise Threshold", &config.rise_threshold, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The 'rise' that is the maximal allowed under height smoothening.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Coast Line", &config.delta_coast_line, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The amount above or below sealine that defines a coast.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Wind str alpha", &config.windstr_alpha, 1.0f, 0.0f, 10.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Alpha value for the beta distribution of wind strenght.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Wind str beta", &config.windstr_beta, 1.0f, 0.0f, 10.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Beta value for the beta distribution of wind strenght.");
                    ImGui::EndTooltip(); }
                ImGui::InputUInt("Method of Biomes", &config.biome_method); //TODO, fix this input
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Methods of biome generation. 1: GMM and probability smoothing, 2: KMeans (should be faster, is not)");
                    ImGui::EndTooltip();
                }
            }

            ImGui::PopItemWidth();

            ImGui::End();
        }

        
        if (!drawHighlightBool) {
            highlight.clear();
            highlightedCell = vor::INVALID_INDEX;
        }


        if (showLoadConfig)
        {
            ImGuiFD::OpenDialog("Choose Dir", ImGuiFDMode_LoadFile, ".");
        }
        else if (showSaveConfig)
        {
            ImGuiFD::OpenDialog("Choose Dir", ImGuiFDMode_SaveFile, ".");
        }
        
        std::string path; // Path to save and load filess
        if (ImGuiFD::BeginDialog("Choose Dir")) {
            if (ImGuiFD::ActionDone()) {
                if (ImGuiFD::SelectionMade()) {
                    path = ImGuiFD::GetSelectionPathString(0);
					std::cout << "Selected: " << path << std::endl;
                    if (showLoadConfig)
                    {
                        config.load_json(path);
                        showLoadConfig = false;
                    }
                    else if (showSaveConfig)
                    {
                        config.save_json(path);
                        showSaveConfig = false;
                    }
                }
                if (ImGuiFD::CanceledOperation())
                {
                    showSaveConfig = false;
                    showLoadConfig = false;
                }
                ImGuiFD::CloseCurrentDialog();
            }
            ImGuiFD::EndDialog();
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

        ImGui::SFML::Render(window);

        window.display();
        
    }
    ImGui::SFML::Shutdown(window);

    return 0;
}

