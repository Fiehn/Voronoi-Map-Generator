#pragma once
#include <iostream>
#include <string>
#include <time.h>
#include <cstdlib>
#include "Voronoi.hpp"
#include "vertex.hpp"

#include "imgui.h"
#include "imgui-SFML.h"

// FOR UI, Create a Biome generator view for an existing Voronoi map

// There is some inspiration to get from the following (particularly for threading):
// https://gitlab.gbar.dtu.dk/s164179/Microbots/blob/dc8b5b4b883fa1fe572fd82d44fbf291d7f81153/SFML-2.5.0/examples/island/Island.cpp

static void loadText(sf::RenderWindow& window, sf::Text& text, int fontsize, std::string& displayText, std::string newText)
{
    displayText = displayText.substr(0, displayText.length() - 3) + "(X)" + "\n" + newText + " (0)";
    text.setString(displayText);
    text.move(0, - fontsize);
    window.clear();
    window.draw(text);
    window.display();
}

static void genWorld(vor::Voronoi& map, GlobalWorldObjects& globals, sf::RenderWindow& window, VertexMap& vertexMap,
    sf::VertexArray& windArrows,
    sf::VertexArray& lines,
    const sf::Font& font,
    const unsigned int& n_convergence_lines,
    const unsigned int& n_biomes,
    const unsigned int& ncellx,
    const unsigned int& ncelly,
    const unsigned int& MAXWIDTH,
    const unsigned int& MAXHEIGHT,
    const float& point_jitter, 
    const unsigned int& npeaks,
    const float& sealevel,
    const float& global_temp_avg,
    const float& delta_max_neg,
    const float& delta_max_pos,
    const float& prob_of_island,
    const float& dist_from_mainland,
    const int& height_method,
    const float& rise_threshold,
    const unsigned int& height_smooth_repeats,
    const int& smooth_method,
    const unsigned int& height_noise_repeats,
    const float& delta_coast_line,
    const unsigned int& temp_smooth_repeats,
    const unsigned int& percepitation_repeats,
    const unsigned int& percepitation_smooth_repeats,
    const unsigned int& kmeans_max_iter,
    const float& windstr_alpha,
    const float& windstr_beta,
    const unsigned int& biome_method,
    unsigned int seed
) {
    // Seed
    if (seed == 0) { seed = time(NULL); }
    std::srand(seed);

    // Globals
    globals.clearGlobals();
    globals.setSeaLevel(sealevel); // RandomBetween(0.4f, 0.6f)
    globals.setGlobalTemp(global_temp_avg);
    globals.generateConvergenceLines(n_convergence_lines, windstr_alpha, windstr_beta);
    lines.clear();
    lines.resize(2 * n_convergence_lines);
    for (int i = 0; i < n_convergence_lines * 2; i++)
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
    std::vector<sf::Color> biomeColors = randomColors(n_biomes);
    for (int i = 0; i < n_biomes; i++) {
        globals.addBiome("Biome" + std::to_string(i), 0.f, 0.f, 0.f, 0.f, 0.f, false, biomeColors[i]);
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
    map.clearMap();
    map.fillMap(ncellx, ncelly, MAXWIDTH, MAXHEIGHT, point_jitter);


    loadText(window, text, 50, loadingText, "Generating Heightmap");
    random_height_gen(map.cells, npeaks, delta_max_neg, delta_max_pos, prob_of_island, dist_from_mainland, height_method);
    loadText(window, text, 50, loadingText, "Smoothing Heightmap");
    smooth_height(map.cells, rise_threshold, height_smooth_repeats, smooth_method);
    loadText(window, text, 50, loadingText, "Adding Noise to Heightmap");
    noise_height(map.cells, height_noise_repeats);
    loadText(window, text, 50, loadingText, "Calculating Height Values");
    calcHeightValues(map.cells, globals, delta_coast_line);
    loadText(window, text, 50, loadingText, "Distance To Oceans");
    closeOceanCell(map.cells, map.points, globals);
    loadText(window, text, 50, loadingText, "Calculating Wind");
    calcWind(map.cells, map.points, MAXHEIGHT, globals);
    loadText(window, text, 50, loadingText, "Calculating River");
    calcRiverStart(map.cells, globals);
    loadText(window, text, 50, loadingText, "Calculating Temperatures");
    calcTemp(map.cells, globals, map.points, MAXHEIGHT);
    smoothTemps(map.cells, temp_smooth_repeats);
    loadText(window, text, 50, loadingText, "Calculating Percepetation");
    calcPercepitation(map.cells, map.points, globals, percepitation_repeats);
    smoothPercepitation(map.cells, percepitation_smooth_repeats);
    loadText(window, text, 50, loadingText, "Calculating Humidity");
    calcHumid(map.cells);
    loadText(window, text, 50, loadingText, "Calculating Biomes");
    calcBiome(map.cells, globals, kmeans_max_iter, biome_method);
    loadText(window, text, 50, loadingText, "Drawing Wind Arrows");
    windArrows.clear();
    windArrows = vor::windArrows(map);
    loadText(window, text, 50, loadingText, "Generating Vertex Buffer");
    vertexMap.clear();
    vertexMap.create(map);
    vertexMap.genVertexMap(map);
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
        sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
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

int main() 
{
    // Initate seed and window size
    std::srand(time(NULL));
    unsigned int seed = 0; // for changing the seed
    unsigned int windowWidth = 2500;
    unsigned int windowHeight = 1500;

    // For map generation
    float point_jitter = 7.f; // How much to jitter the points after grid placement
    unsigned int ncellx = 150; // Number of cells in x direction
    unsigned int ncelly = 100; // Number of cells in y direction

    // Height generation
    unsigned int npeaks = 10; // Number of peaks to generate in the heightmap
    float delta_max_neg = 0.04; // The maximum amount of random height added in the negative direction
    float delta_max_pos = 0.02; // The maximum amount of random height added in the positive direction
    float prob_of_island = 0.01; // small probability of random height increase when away from mainland 
    float dist_from_mainland = 1.0; // The distance from the mainland where the probability of random height increase begins, Represented by the sum of height of all neighbors
    int height_method = 1; // Method 1 is random, method 2 is first in first out, needs more methods (Simplex, diamond, perlin, etc)
    float rise_threshold = 0.09; // The minimum rise value where a cell height is smoothed 
    unsigned int height_smooth_repeats = 15; // amount of height smoothing iterations
    int smooth_method = 1; // method 1 is random the other is front
    unsigned int height_noise_repeats = 2; // amount of height noise iterations, happens after smoothing
    float delta_coast_line = 0.05; // the range around sealevel that is considered coast (below and above)

    // Temperature
    float global_temp_avg = RandomBetween(25.f, 45.f); // not the actual average but a value that determines the temperature range
    unsigned int temp_smooth_repeats = 2; // amount of temperature smoothing iterations

    // Sealevel
    float sealevel = RandomBetween(0.4f, 0.6f); // The height at which the ocean starts
    
    // Percepitation
    unsigned int percepitation_repeats = 1; // amount of percepitation iterations (NEEDs to be above 1)
    unsigned int percepitation_smooth_repeats = 2; // amount of percepitation smoothing iterations
    
    // Biomes
    unsigned int kmeans_max_iter = 5; // The maximum amount of iterations for the kmeans algorithm
    unsigned int n_biomes = 8; // The amount of clusters for the kmeans algorithm (amount of biomes)
    unsigned int biome_method = 1; // Method 1 is GMM and method 2 is Kmeans

    // Wind
    unsigned int n_convergence_lines = 5; // The amount of convergence lines to generate Needs 
    float windstr_alpha = 2;
    float windstr_beta = 2;

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

    // Create the view
    sf::Vector2f oldPos;
    bool moving = false;
    bool drawLines = false; // Set to true to draw the convergence lines of wind direction
    bool wind = false; // Set to true to draw the wind direction
    bool highlightBool = false; // Set to true to highlight a cell
    int mapType = 0; int mapTypeOld = 0;
    bool newMap = false; // Get window to draw new map

    // Retrieve the window's default view
    float zoom = 1;
    sf::View view = window.getDefaultView();

    sf::Clock deltaClock;

    // Generate the actual map:
    genWorld(map, globals, window, vertexMap,
        windArrows, lines, font,
        n_convergence_lines,
        n_biomes, ncellx, ncelly,
        windowWidth, windowHeight,
        point_jitter, npeaks, sealevel,
        global_temp_avg, delta_max_neg,
        delta_max_pos, prob_of_island,
        dist_from_mainland, height_method,
        rise_threshold, height_smooth_repeats,
        smooth_method, height_noise_repeats,
        delta_coast_line, temp_smooth_repeats,
        percepitation_repeats, percepitation_smooth_repeats,
        kmeans_max_iter, windstr_alpha, windstr_beta,biome_method, seed);

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
                        windArrows, lines, font,
                        n_convergence_lines,
                        n_biomes, ncellx, ncelly,
                        windowWidth, windowHeight,
                        point_jitter, npeaks, sealevel,
                        global_temp_avg, delta_max_neg,
                        delta_max_pos, prob_of_island,
                        dist_from_mainland, height_method,
                        rise_threshold, height_smooth_repeats,
                        smooth_method, height_noise_repeats,
                        delta_coast_line, temp_smooth_repeats,
                        percepitation_repeats, percepitation_smooth_repeats,
                        kmeans_max_iter, windstr_alpha, windstr_beta, biome_method, seed);
                }

                // Draw Lines
                else if (event.key.code == sf::Keyboard::L) { drawLines = !drawLines; }

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
                else if (event.key.code == sf::Keyboard::Comma) { wind = !wind; }

                // Activate Cell Highlighting
                else if (event.key.code == sf::Keyboard::H) { highlightBool = !highlightBool; highlight.clear(); }

				break;

            // Mouse Movement
            case sf::Event::MouseMoved:
            {
                // If no button is down, we are not moving the view
                if (!moving) {
                    // Highlight Cells if active 
                    if (highlightBool)
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
                if ((view.getCenter().x + deltaPos.x) + zoom * windowWidth / 2 > windowWidth || (view.getCenter().x + deltaPos.x) - zoom * windowWidth / 2 < 0) {
                    view_center.x = view.getCenter().x;
                }
                if ((view.getCenter().y + deltaPos.y) + zoom * windowHeight / 2 > windowHeight || (view.getCenter().y + deltaPos.y) - zoom * windowHeight / 2 < 0) {
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
                    zoom = std::min(1.f, zoom + .07f);
                }
                else if (event.mouseWheelScroll.delta >= 1)
                {
                    zoom = std::max(.1f, zoom - .07f);
                }

                // Update our view
                view.setSize(window.getDefaultView().getSize());
                view.zoom(zoom);

                // Make sure that the view is contained inside the map
                sf::Vector2f view_center = view.getCenter();
                if ((view.getCenter().x) + zoom * windowWidth / 2 > windowWidth) {
                    view_center.x = windowWidth - zoom * windowWidth / 2;
                }
                if ((view.getCenter().x) - zoom * windowWidth / 2 < 0) {
                    view_center.x = zoom * windowWidth / 2;
                }
                if ((view.getCenter().y) + zoom * windowHeight / 2 > windowHeight) {
                    view_center.y = windowHeight - zoom * windowHeight / 2;
                }
                if (view.getCenter().y - zoom * windowHeight / 2 < 0) {
                    view_center.y = zoom * windowHeight / 2;
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
                wind = false;
				break;
			case 1:
				drawTempMap(map, vertexMap);
                wind = false;
				break;
			case 2:
				drawBiomeMap(map, globals, vertexMap);
                wind = false;
				break;
			case 3:
				drawPercepitationMap(map, vertexMap);
                wind = false;
				break;
            case 4:
				drawWindMap(map, vertexMap);
                wind = true;
				break;
			}
		}

        ImGui::Checkbox("Draw Lines", &drawLines);
        ImGui::Checkbox("Wind Arrows", &wind);
        ImGui::Checkbox("Highlight Cells", &highlightBool);


        ImGui::Text("Number of cells: %d", map.cells.size());
        ImGui::Text("Number of biomes: %d", globals.biomes.size());
        ImGui::Text("Sealevel: %.2f", globals.seaLevel);
        ImGui::Text("Global Temperature: %.2f", globals.globalTempAvg);
        ImGui::Text("Global Precipitation: %.2f", globals.globalPercepitation);
        ImGui::Text("Global Snow Line: %.2f", globals.globalSnowline);


        // Display the temp, percepitation, and elevation, biome of the highlighted cell at the same position
        if (highlightedCell != vor::INVALID_INDEX) {
			const Cell& cell = map.cells[highlightedCell];
			ImGui::Text("Cell %d", highlightedCell);
			ImGui::Text("Temp: %.2f", cell.temp);
            ImGui::Text("Precipitation: %.2f", cell.percepitation);
            ImGui::Text("Elevation: %.2f", cell.height); ImGui::SameLine();
            ImGui::Text("Rise: %.2f", cell.rise);
            ImGui::Text("Distance to Ocean: %.2f", cell.distToOcean);
            ImGui::Text("Coast Cell: %.d", cell.coastBool);
            ImGui::Text("Ocean Cell: %.d", cell.oceanBool);
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
                ImGui::SameLine();
                ImGui::Text("Temp: %.2f", biome.avgTemp);
                ImGui::SameLine();
                ImGui::Text("Precip: %.2f", biome.avgRain);
                ImGui::SameLine();
                ImGui::Text("Elevation: %.2f", biome.avgElevation);
                ImGui::SameLine();
                ImGui::Text("Wind Str: %.2f", biome.avgWindStr);
                ImGui::SameLine();
                ImGui::Text("Ocean: %d", biome.isOcean);
                ImGui::Text("Size: %d", biome.numCells);
                
                ImGui::PopID();


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
        }

        ImGui::Checkbox("Draw New Map", &newMap);

        if(ImGui::Button("Switch origin of vertexMap")) { vertexMap.switchOrigin(map); };
        if (ImGui::Button("Check vertexMap")) { std::cout << vertexMap.useVertexBuffer << " : Array: " << vertexMap.vertexArray.getVertexCount() << " : Buffer: " << vertexMap.vertexBuffer.getVertexCount() << std::endl; };

        ImGui::End();

        if (newMap)
        {
            ImGui::Begin("New Map Controls");
            // push a color on the generate new map button

            if (ImGui::Button("Generate New Map", {200,50})) {
                genWorld(map, globals, window, vertexMap,
                    windArrows, lines, font,
                    n_convergence_lines,
                    n_biomes, ncellx, ncelly,
                    windowWidth, windowHeight,
                    point_jitter, npeaks, sealevel,
                    global_temp_avg, delta_max_neg,
                    delta_max_pos, prob_of_island,
                    dist_from_mainland, height_method,
                    rise_threshold, height_smooth_repeats,
                    smooth_method, height_noise_repeats,
                    delta_coast_line, temp_smooth_repeats,
                    percepitation_repeats, percepitation_smooth_repeats,
                    kmeans_max_iter, windstr_alpha, windstr_beta, biome_method ,seed);
                newMap = false;
            }

            ImGui::Text("The map will have around %.d cells", ncellx * ncelly);

            ImGui::PushItemWidth(150.f);

            ImGui::InputUInt("Cells x", &ncellx); 
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Number of cells in the x direction. \nKeep at least above 100 or it starts to break down.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Cells y", &ncelly);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Number of cells in the y direction. \nKeep at least above 100 or it starts to break down.");
                ImGui::EndTooltip(); }

            ImGui::DragFloat("Jitter of the cells", &point_jitter,0.5f,0.0f,10.f);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount each cell is jittered from their grid position");
                ImGui::EndTooltip(); }

            ImGui::DragFloat("Sea Level", &sealevel, 0.01f, 0.0f, 1.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("The height of the sealevel across map, between 0 and 1.");
                ImGui::EndTooltip(); }

            ImGui::DragFloat("Global Temp Avg", &global_temp_avg, 0.5f, -30.0f, 100.0f);
            if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("This is an initializer for global temperature modifier, \naround 30-50 will give realistic values (to earth) depending on the size of the map.");
				ImGui::EndTooltip(); }

            ImGui::InputUInt("Number of Peaks", &npeaks);
            if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text("Number of peaks to generate on the map.");
				ImGui::EndTooltip(); }

            ImGui::InputUInt("Nr of Convergence Lines", &n_convergence_lines);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Defines lines that split the prevailing winds. \nDirections and strengths are then concluded randomly, Earth has 6 zones so input 6.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Amount of Biomes", &n_biomes);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount of biomes to generate initially. \nLarge maps will often have a lot of ocean biomes.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Method of Biomes", &biome_method); //TODO, fix this input
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Methods of biome generation. 1: GMM and probability smoothing, 2: KMeans (should be faster, is not)");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Height Smooths", &height_smooth_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Amount of times the heihgt map is smoothed. \nShould be above 2 ideally more. \nThis is relatively intensive.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Height Noisers", &height_noise_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("This is the amount of times the height values are renoised after the smoothing, \nthis gives a more realistic height map. \nShould not be used more times than smoothing.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Temp Smooths", &temp_smooth_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("This is the amount of times temperature is smoothed out.");
                ImGui::EndTooltip(); }

            ImGui::InputUInt("Percepitations", &percepitation_repeats);
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("This is the amount of times percepitation is calculated. \nShould not be more than 1 unless you want high contrast.");
                ImGui::EndTooltip(); }
            if (percepitation_repeats == 0) { percepitation_repeats = 1; }

            ImGui::InputUInt("Percepitation Smooths", &percepitation_smooth_repeats);
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
                ImGui::DragFloat("Delta Max Neg", &delta_max_neg, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The maximum amount of random height added in the negative direction. \nIncreasing will make the map more steepely falling");
                    ImGui::EndTooltip();
                }
                ImGui::DragFloat("Delta Max Pos", &delta_max_pos, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The maximum amount of random height added in the positive direction. \nIncreasing will make the map more steepely falling");
                    ImGui::EndTooltip();}
                ImGui::DragFloat("Islands", &prob_of_island, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("small probability of random height increase when away from mainland. \nIncrease to make more islands at a short distance from shore.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Distance from land", &dist_from_mainland, 0.04f, 0.0f, 5.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The distance from the mainland where the probability of random height increase begins,\n Represented by the sum of height of all neighbors");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Rise Threshold", &rise_threshold, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The 'rise' that is the maximal allowed under height smoothening.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Coast Line", &delta_coast_line, 0.01f, 0.0f, 1.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("The amount above or below sealine that defines a coast.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Wind str alpha", &windstr_alpha, 1.0f, 0.0f, 10.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Alpha value for the beta distribution of wind strenght.");
                    ImGui::EndTooltip(); }
                ImGui::DragFloat("Wind str beta", &windstr_beta, 1.0f, 0.0f, 10.0f);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Beta value for the beta distribution of wind strenght.");
                    ImGui::EndTooltip(); }
            }

            ImGui::PopItemWidth();

            ImGui::End();
        }

        if (!highlightBool) {
            highlight.clear();
            highlightedCell = vor::INVALID_INDEX;
        }

        window.clear();
        
        vertexMap.draw(window);
        
        if (drawLines) {
			window.draw(lines);
		}
        if (wind) {
			window.draw(windArrows);
		}
        if (highlightedCell != vor::INVALID_INDEX) {
			window.draw(highlight);
		}

        ImGui::SFML::Render(window);

        window.display();
        
    }
    ImGui::SFML::Shutdown(window);

    return 0;
}

