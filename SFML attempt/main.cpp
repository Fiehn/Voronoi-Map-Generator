#include <SFML/Graphics.hpp>
#include "Voronoi.hpp"
#include "cell.hpp"
#include <time.h>
#include <cstdlib>
#include <iostream>

// There might be a problem with neighbors.. There are holes in the height map??



int main()
{
    std::srand(time(NULL));
    int windowWidth = 2500;
    int windowHeight = 1500;

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "SFML");
    window.setFramerateLimit(27); // For now there is no reason to have even this high framerate


    vor::Voronoi map(200, 150, windowWidth, windowHeight, 10.f);
    GlobalWorldObjects globals;
    

    random_height_gen(map.cells, 7, 0.04, 0.02, 0.01, 1.0, "Random");
    smooth_height(map.cells,0.09,15,"Random");
    noise_height(map.cells, 2);
    
    calcHeightValues(map.cells, globals, 0.05);

    calcRiverStart(map.cells, globals);


    // Initialization for drawing the cells
    sf::RenderTexture bgMap;
    bgMap.create(windowWidth, windowHeight);

    for (int i = 0; i < map.cells.size(); i++) {
        
        sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255/3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
        // sf::Color color(0,0,255 * map.cells[i].riverBool, 255);

        if (map.cells[i].vertex.size() == 0) { continue; };
        sf::VertexArray T(sf::TriangleFan, map.cells[i].vertex.size() + 1);

        for (int j = 0; j < map.cells[i].vertex.size(); j++) 
        {
            T[j].position = map.voronoi_points[map.cells[i].vertex[j]];
            T[j].color = color;
        }


        T[map.cells[i].vertex.size()].position = map.voronoi_points[map.cells[i].vertex[0]];
        T[map.cells[i].vertex.size()].color = color;

        bgMap.draw(T);
    }

    sf::Vector2f oldPos;
    bool moving = false;

    float zoom = 1;
    // Retrieve the window's default view
    sf::View view = window.getDefaultView();

    bgMap.display();
    sf::Sprite background(bgMap.getTexture());

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
            case sf::Event::MouseButtonPressed:
                // Mouse button is pressed, get the position and set moving as active
                if (event.mouseButton.button == 0) {
                    moving = true;
                    oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                }
                if (event.mouseButton.button == 1) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    int cellIndex = map.getCellIndex(mousePos);
                    // display
                    std::cout << "Cell index: " << cellIndex << std::endl;
                    std::cout << "Cell height: " << map.cells[cellIndex].height << std::endl;
                    std::cout << "Cell riverBool: " << map.cells[cellIndex].riverBool << std::endl;
                    std::cout << "Cell oceanBool: " << map.cells[cellIndex].oceanBool << std::endl;
                    std::cout << "Cell snowBool: " << map.cells[cellIndex].snowBool << std::endl;
                    std::cout << "Cell lakeBool: " << map.cells[cellIndex].lakeBool << std::endl;
                    // neighbors
                    std::cout << "Cell neighbors: " << map.cells[cellIndex].neighbors.size() << std::endl;
                }
                break;

            case sf::Event::MouseButtonReleased:

                // Mouse button is released, no longer move
                if (event.mouseButton.button == 0) {
                    moving = false;
                }
                break;

            case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Escape)
				    window.close();
                else if (event.key.code == sf::Keyboard::A)
                { // activate text-box
                    continue;
                }
				break;

            case sf::Event::MouseMoved:
            {
                // Ignore mouse movement unless a button is pressed (see above)
                if (!moving) {
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
            case sf::Event::MouseWheelScrolled:
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

        window.clear();
        
        window.draw(background);

        window.display();        
    }

    return 0;
}

