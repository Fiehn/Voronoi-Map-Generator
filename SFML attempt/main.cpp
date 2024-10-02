#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <time.h>
#include <cstdlib>
#include "Voronoi.hpp"

#include "imgui.h"
#include "imgui-SFML.h"

// FOR UI, create new game and make all values editable, also tighten up the code to make it more readable 

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

static void genWorld(GlobalWorldObjects& globals, vor::Voronoi& map, const int ncellx, const int ncelly, const int MAXWIDTH, const int MAXHEIGHT, const float point_jitter, int peaks,sf::RenderWindow& window)
{
    std::string loadingText = "Initializing (0)";
    sf::Font font;
    if (!font.loadFromFile("Fonts/arial.ttf"))
    {
        std::cout << "Could not load font" << std::endl;
    }
    sf::Text text(loadingText, font, 50);
    text.setFillColor(sf::Color::White);
    text.setPosition(MAXWIDTH / 2 - text.getGlobalBounds().width / 2, MAXHEIGHT / 2 - text.getGlobalBounds().height / 2);
    // Draw the loading screen
    window.clear();
    window.draw(text);
    window.display();

    map.clearMap();
    map.fillMap(ncellx, ncelly, MAXWIDTH, MAXHEIGHT, point_jitter);
    globals.clearGlobals();
    globals.generateConvergenceLines(5);
    globals.setSeaLevel(RandomBetween(0.4f, 0.6f));
    globals.setGlobalTemp(RandomBetween(25.f, 45.f));
    std::cout << "Wind Directions: " << globals.windDirection[0] << " " << globals.windDirection[1] << " " << globals.windDirection[2] << " " << globals.windDirection[3] << std::endl;
    std::cout << "Global Avereage Temp: " << globals.globalTempAvg << " " << "Sea Level: " << globals.seaLevel << std::endl;

    loadText(window, text, 50, loadingText, "Generating Heightmap");
    random_height_gen(map.cells, peaks, 0.04, 0.02, 0.01, 1.0, 1);
    loadText(window, text, 50, loadingText, "Smoothing Heightmap");
    smooth_height(map.cells, 0.09, 15, 1);
    loadText(window, text, 50, loadingText, "Generating Noise");
    noise_height(map.cells, 2);
    loadText(window, text, 50, loadingText, "Calculating Heights");
    calcHeightValues(map.cells, globals, 0.05);
    loadText(window, text, 50, loadingText, "Distance To Oceans");
    closeOceanCell(map.cells, map.points, globals);
    loadText(window, text, 50, loadingText, "Calculating Wind");
    calcWind(map.cells,map.points, MAXHEIGHT, globals);
    loadText(window, text, 50, loadingText, "Calculating Temperature");
    calcTemp(map.cells, globals, map.points, MAXHEIGHT);
    smoothTemps(map.cells, 1);
    loadText(window, text, 50, loadingText, "Calculating Percepetation");
    calcPercepitation(map.cells, map.points, globals);
    smoothPercepitation(map.cells, 1);
    loadText(window, text, 50, loadingText, "Calculating Biomes");
    calcBiome(map.cells, globals);
    loadText(window, text, 50, loadingText, "Calculating Rivers");
    calcRiverStart(map.cells, globals);
    return;
}
static sf::VertexBuffer genBuffer(vor::Voronoi& map)
{
	sf::VertexBuffer vertexBuffer(sf::Triangles, sf::VertexBuffer::Dynamic);
	vertexBuffer.create(map.vertexCount * 3);
	
	// Attempt VertexBuffer
    if (sf::VertexBuffer::isAvailable()) {
		std::cout << "Vertex buffer is available" << std::endl;
	}
    else {
		std::cout << "Vertex buffer is not available" << std::endl;
		vertexBuffer;
	}
	// Fill vertex buffer / draw triangles from map.vertices
    for (std::size_t i = 0; i < map.cells.size(); i++) {
		sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
			map.vertices[j].color = color;
		}
	}
	vertexBuffer.update(map.vertices.data());
	return vertexBuffer;
}
static sf::VertexArray genArray(vor::Voronoi& map)
{
    sf::VertexArray vertexArray(sf::Triangles, map.vertexCount * 3);

    for (std::size_t i = 0; i < map.cells.size(); i++) {
		sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
			map.vertices[j].color = color;
		}
	}
    // Fill vertex array / draw triangles from map.vertices
    for (std::size_t i = 0; i < map.vertices.size(); i++) {
        vertexArray.append(map.vertices[i]);
    }
    return vertexArray;
}

static void updateVertex(vor::Voronoi& map, sf::VertexArray& vertexArray, sf::VertexBuffer& vertexBuffer, bool useVertexBuffer)
{
    if (useVertexBuffer)
    {
		vertexBuffer.update(map.vertices.data());
	}
    else
    {
		vertexArray.clear();
        for (std::size_t i = 0; i < map.vertices.size(); i++) {
			vertexArray.append(map.vertices[i]);
		}
	}
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

static void drawTempMap(vor::Voronoi& map, bool& temp, sf::VertexArray& vertexArray, sf::VertexBuffer& vertexBuffer, bool useVertexBuffer) {
    temp = true;
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(255, 255 / 2 + clamp(5 * map.cells[i].temp, 255 / 2, -255), 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    updateVertex(map, vertexArray, vertexBuffer, useVertexBuffer);
}

static void drawBiomeMap(vor::Voronoi& map, GlobalWorldObjects globals, bool& biomes, sf::VertexArray& vertexArray, sf::VertexBuffer& vertexBuffer, bool useVertexBuffer)
{
    biomes = true;
    for (size_t i = 0; i < map.cells.size(); i++)
    {

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = globals.biomes[map.cells[i].biome].color;
        }
    }
    updateVertex(map, vertexArray, vertexBuffer, useVertexBuffer);
}

static void drawPercepitationMap(vor::Voronoi& map, bool& percep, sf::VertexArray& vertexArray, sf::VertexBuffer& vertexBuffer, bool useVertexBuffer)
{
    percep = true;
    for (size_t i = 0; i < map.cells.size(); i++)
    {
        sf::Color color(0, clamp(5 * map.cells[i].percepitation, 255, 0), 0, 255);

        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
        {
            map.vertices[j].color = color;
        }
    }
    updateVertex(map, vertexArray, vertexBuffer, useVertexBuffer);
}

static void drawHeightMap(vor::Voronoi& map, sf::VertexArray& vertexArray, sf::VertexBuffer& vertexBuffer, bool useVertexBuffer)
{
    for (std::size_t i = 0; i < map.cells.size(); i++) {
        sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
        for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
            map.vertices[j].color = color;
        }
    }
    updateVertex(map, vertexArray, vertexBuffer, useVertexBuffer);
}

int main() 
{
    // Initialize the random seed and window
    std::srand(time(NULL));
    int windowWidth = 2500;
    int windowHeight = 1500;

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "SFML");
    window.setFramerateLimit(27); // For now there is no reason to have even this high framerate
    ImGui::SFML::Init(window);

    // Create the global world objects
    GlobalWorldObjects globals;
    
    
    // Generate Global Lines // NEEDS TO BE MOVED TO GLOBAL OBJECTS
    globals.generateConvergenceLines(5);
    sf::VertexArray lines(sf::Lines, 10);
    for (int i = 0; i < 10; i++)
    {
        if (i % 2 == 0)
        {
            lines.append(sf::Vertex(sf::Vector2f(0, globals.convergenceLines[std::floor(i / 2)] * windowHeight), sf::Color::Green));
        }
        else 
        {
            lines.append(sf::Vertex(sf::Vector2f(windowWidth, globals.convergenceLines[std::floor(i / 2)] * windowHeight), sf::Color::Green));
        }
	}

    // Print initials:
    std::cout << "Wind Directions: " << globals.windDirection[0] << " " << globals.windDirection[1] << " " << globals.windDirection[2] << " " << globals.windDirection[3] << " " << globals.windDirection[4] << std::endl;
    std::cout << "Global Avereage Temp: " << globals.globalTempAvg << " " << "Sea Level: " << globals.seaLevel << std::endl;

    // Create empty Biomes // NEEDS to be reworked to work better with K-means, and to be moved to global objects
    std::vector<sf::Color> biomeColors = randomColors(10);
    for (int i = 0; i < 10; i++) {
        globals.addBiome("Biome" + std::to_string(i), 0.f, 0.f, 0.f, 0.f, 0.f, false, { true }, biomeColors[i]);
    }
    biomeColors.clear();
    

    // Create the loading screen
    std::string loadingText = "Initializing (0)";
    sf::Font font;
    if (!font.loadFromFile("Roboto-Medium.ttf"))
    {
		std::cout << "Could not load font" << std::endl;
	}
    sf::Text text(loadingText, font, 50);
    text.setFillColor(sf::Color::White);
    text.setPosition(windowWidth / 2 - text.getGlobalBounds().width / 2, windowHeight / 2 - text.getGlobalBounds().height / 2);
    // Draw the loading screen
    window.clear();
    window.draw(text);
    window.display();

    // Create the map
    vor::Voronoi map(200, 150, windowWidth, windowHeight, 8.f);
    
    loadText(window,text,50,loadingText,"Generating Heightmap");
    random_height_gen(map.cells, 10, 0.04, 0.02, 0.01, 1.0, 1);

    loadText(window,text,50,loadingText,"Smoothing Heightmap");
    smooth_height(map.cells, 0.09, 15, 1);

    loadText(window,text,50,loadingText,"Adding Noise to Heightmap");
    noise_height(map.cells, 2);

    loadText(window,text,50,loadingText,"Calculating Height Values");
    calcHeightValues(map.cells, globals, 0.05);

    loadText(window,text,50,loadingText,"Distance To Oceans");
    closeOceanCell(map.cells, map.points, globals);

    loadText(window,text,50,loadingText,"Calculating Wind");
    calcWind(map.cells, map.points, windowHeight, globals);

    loadText(window,text,50,loadingText,"Calculating River");
    calcRiverStart(map.cells, globals);

    loadText(window, text, 50, loadingText, "Calculating Temperatures");
    calcTemp(map.cells, globals, map.points, windowHeight);
    smoothTemps(map.cells, 2);

    loadText(window,text,50,loadingText,"Calculating Percepetation");
    calcPercepitation(map.cells, map.points, globals, 1);
    smoothPercepitation(map.cells, 1);

    loadText(window,text,50,loadingText,"Calculating Humidity");
    calcHumid(map.cells);

    loadText(window,text,50,loadingText,"Calculating Biomes");
    calcBiome(map.cells, globals);
    std::cout << "Biomes: " << std::endl;
    for (int i = 0; i < globals.biomes.size(); i++)
    {
        std::cout << globals.biomes[i].name << ": " << std::endl;
        std::cout << "Color: " << colorName(globals.biomes[i].color) << ", Temperature: " << globals.biomes[i].getAvgTemp() << ", Percepitation: " << globals.biomes[i].getAvgRain() << ", Humidity: " << globals.biomes[i].getAvgHumidity() << ", Height: " << globals.biomes[i].getAvgElevation() << ", Wind Str: " << globals.biomes[i].getAvgWindStr() << ", Ocean: " << globals.biomes[i].isOcean << std::endl;
	}
    
    loadText(window,text,50,loadingText,"Drawing Wind Arrows");
    sf::VertexArray windArrows = vor::windArrows(map);

    loadText(window,text,50,loadingText,"Generating Vertex Buffer");

    bool useVertexBuffer = sf::VertexBuffer::isAvailable;

    // Create the vertex map
    sf::VertexBuffer vertexBuffer;
    sf::VertexArray vertexArray;
    if (useVertexBuffer)
    {
        vertexBuffer = genBuffer(map);
    }
    else
    {
        vertexArray = genArray(map);
    }

    // Create the view
    sf::Vector2f oldPos;
    bool moving = false;
    bool drawLines = false; // Set to true to draw the convergence lines of wind direction
    bool wind = false; // Set to true to draw the wind direction
    bool temp = false; // Set to true to draw the temperature lines
    bool percep = false; // Set to true to draw the percepitation lines
    bool biomes = false; // Set to true to draw the biomes
    bool highlightBool = false; // Set to true to highlight a cell
    int mapType = 0; int mapTypeOld = 0;

    // Retrieve the window's default view
    float zoom = 1;
    sf::View view = window.getDefaultView();

    // Create empty highlighted cell
    std::size_t highlightedCell = vor::INVALID_INDEX;
    sf::VertexArray highlight(sf::LineStrip, 5);

    sf::Clock deltaClock;
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
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    std::size_t cellIndex = map.getCellIndex(mousePos);
                    if(cellIndex != vor::INVALID_INDEX) {
                        std::cout << "ID: " << cellIndex << " Height: " << map.cells[cellIndex].height << " riverBool: " << map.cells[cellIndex].riverBool << " oceanBool: " << map.cells[cellIndex].oceanBool << " snowBool: " << map.cells[cellIndex].snowBool << " lakeBool: " << map.cells[cellIndex].lakeBool << std::endl;
                        std::cout << "Coordinates: " << map.points[cellIndex].x << " " << map.points[cellIndex].y << std::endl;
                        std::cout << "Temp: " << map.cells[cellIndex].temp << " Percip: " << map.cells[cellIndex].percepitation << " Humidity: " << map.cells[cellIndex].humidity << std::endl;
                        std::cout << "Wind direction: " << map.cells[cellIndex].windDir << " Wind speed: " << map.cells[cellIndex].windStr << std::endl;
                        std::cout << "Distance to Ocean: " << map.cells[cellIndex].distToOcean << std::endl;
                        std::cout << "Biome: " << globals.biomes[map.cells[cellIndex].biome].name << std::endl;
                        std::cout << std::endl;
                    }
                    else {
                        std::cout << "Out of Bounds!" << std::endl;
                    }
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
                if (event.key.code == sf::Keyboard::Escape)
                {
                    window.close();
                    break;
                }

                // Debug:: Draw Cells in Triangles
                else if (event.key.code == sf::Keyboard::A)
                { // paint the cells triangles in vertexBuffer
                    
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    
                    std::size_t cellIndex = map.getCellIndex(mousePos);
                    if (cellIndex == vor::INVALID_INDEX) {
						break;
					}
                    unsigned int offset = map.cells[cellIndex].vertex_offset;
                    int n_vertices = map.cells[cellIndex].vertex.size();

                    // now change color on vertex
                    sf::Color color(100,100,100,255);
                    for (int i = 0; i < n_vertices * 3; i++)
                    {
                        map.vertices[offset + i].color = color;
                    }
                    // TODO: update only the part of the buffer that has changed
                    updateVertex(map,vertexArray,vertexBuffer,useVertexBuffer); // There is no need to update the whole buffer, only the part that has changed (offset, n_vertices)

                }

                // Percepitation map
                else if (event.key.code == sf::Keyboard::P) 
                {
                    drawPercepitationMap(map, percep, vertexArray, vertexBuffer, useVertexBuffer);
                }

                // Generate a new Map
                else if (event.key.code == sf::Keyboard::N)
                {
                    // Delete the map and draw a new one
                    
                    genWorld(globals, map, 200, 150, windowWidth, windowHeight, 8.f, 10, window);
                    if (useVertexBuffer)
                    {
                        vertexBuffer = genBuffer(map);
                    }
                    else
                    {
                        vertexArray = genArray(map);
                    }
                    windArrows = vor::windArrows(map); // NEEEEEDS to be fixed so that it works with the new map
                }

                // Draw Lines
                else if (event.key.code == sf::Keyboard::L)
                {
                    // Display lines
                    drawLines = !drawLines;
                }

                // Wind Map
                else if (event.key.code == sf::Keyboard::W)
                {
                    if (wind == false)
                    {
						wind = true;
                        for (size_t i = 0; i < map.cells.size(); i++)
                        {
                            sf::Color color(255 * map.cells[i].windDir / 360, 255 * map.cells[i].windStr, 0, 255);

                            for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++)
                            {
                                map.vertices[j].color = color;
                            }
                        }
                        updateVertex(map,vertexArray,vertexBuffer,useVertexBuffer);
					}
                    else
                    {
						wind = false;
                        for (std::size_t i = 0; i < map.cells.size(); i++) {
                            sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
                            for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
                                map.vertices[j].color = color;
                            }
                        }
                        updateVertex(map, vertexArray, vertexBuffer, useVertexBuffer);
                    }
                }

                // Temperature map
                else if (event.key.code == sf::Keyboard::T) { drawTempMap(map, temp, vertexArray, vertexBuffer, useVertexBuffer); }

                // Display Biomes
                else if (event.key.code == sf::Keyboard::B) { drawBiomeMap(map, globals, biomes, vertexArray, vertexBuffer, useVertexBuffer); }
                
                // activate arrows for wind direction
                else if (event.key.code == sf::Keyboard::Comma)
                { 
                    wind = !wind;
                }

                // Activate Cell Highlighting
                else if (event.key.code == sf::Keyboard::H) 
                {
                    highlightBool = !highlightBool;
                    highlight.clear();
                }

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

        ImGui::Begin("Hello, world!");
        if (ImGui::Button("Temperature Map")) { drawTempMap(map, temp, vertexArray, vertexBuffer, useVertexBuffer); };
        ImGui::SameLine();
        if (ImGui::Button("Biome Map")) { drawBiomeMap(map, globals, biomes, vertexArray, vertexBuffer, useVertexBuffer); };
        ImGui::SameLine();
        if (ImGui::Button("Percepitation Map")) { drawPercepitationMap(map, percep, vertexArray, vertexBuffer, useVertexBuffer); };


        if (ImGui::BeginTable("Map Chooser",4))
        {
            // Radio Buttons for the different maps
            ImGui::TableNextColumn();
            ImGui::RadioButton("Height", &mapType, 0);
            ImGui::TableNextColumn();
            ImGui::RadioButton("Temperature", &mapType, 1);
            ImGui::TableNextColumn();
            ImGui::RadioButton("Biome", &mapType, 2);
            ImGui::TableNextColumn();
            ImGui::RadioButton("Percepitation", &mapType, 3);
            ImGui::EndTable();
        }
        if (mapType != mapTypeOld) {
            std::cout << "Map Type: " << mapType << std::endl;
			mapTypeOld = mapType;
            switch (mapType) {
			case 0:
				drawHeightMap(map, vertexArray, vertexBuffer, useVertexBuffer);
				break;
			case 1:
				drawTempMap(map, temp, vertexArray, vertexBuffer, useVertexBuffer);
				break;
			case 2:
				drawBiomeMap(map, globals, biomes, vertexArray, vertexBuffer, useVertexBuffer);
				break;
			case 3:
				drawPercepitationMap(map, percep, vertexArray, vertexBuffer, useVertexBuffer);
				break;
			}
		}
        

        ImGui::Checkbox("Draw Lines", &drawLines);
        ImGui::Checkbox("Wind Arrows", &wind);
        ImGui::Checkbox("Highlight Cells", &highlightBool);


        // Display the number of cells, nr of biomes same line
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
            ImGui::Text("Elevation: %.2f", cell.height);
            const Biome& biome = globals.biomes[cell.biome];
            ImVec4 color = ImVec4(biome.color.r / 255.0f, biome.color.g / 255.0f, biome.color.b / 255.0f, 1.0f);
            ImGui::Text("Biome: %s", biome.name.c_str());
            ImGui::SameLine();
            ImGui::ColorEdit4("", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip);
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
                ImGui::PopID();

                if (color[0] != biome.color.r / 255.0f || color[1] != biome.color.g / 255.0f || color[2] != biome.color.b / 255.0f) {
					change = false;
                    biome.color.r = color[0] * 255;
                    biome.color.g = color[1] * 255;
                    biome.color.b = color[2] * 255;
				}
            }

            if (!change) {
				drawBiomeMap(map, globals, change, vertexArray, vertexBuffer, useVertexBuffer);
			}
        }

        ImGui::End();

        if (!highlightBool) {
            highlight.clear();
            highlightedCell = vor::INVALID_INDEX;
        }

        window.clear();
        
        // TODO: Draw only the cells that are visible in the window (use the view to determine which cells are visible) (Low priority since draws are not the bottleneck)
        if (useVertexBuffer)
        {
            window.draw(vertexBuffer);
        }
        else
        {
            window.draw(vertexArray);
        }
        
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

