#include <SFML/Graphics.hpp>
#include "Voronoi.hpp"
#include "cell.hpp"
#include <time.h>
#include <cstdlib>
#include <iostream>

// There might be a problem with neighbors.. There are holes in the height map??
// https://gitlab.gbar.dtu.dk/s164179/Microbots/blob/dc8b5b4b883fa1fe572fd82d44fbf291d7f81153/SFML-2.5.0/examples/island/Island.cpp


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


    sf::VertexBuffer vertexBuffer(sf::Triangles, sf::VertexBuffer::Dynamic);
    vertexBuffer.create(map.vertexCount * 3);
    // Attempt VertexBuffer
    if (sf::VertexBuffer::isAvailable()) {
        std::cout << "Vertex buffer is available" << std::endl;
    }
    else {
        std::cout << "Vertex buffer is not available" << std::endl;
    }

    // Fill vertex buffer / draw triangles
    unsigned int offset = 0;
    for (size_t i = 0; i < map.cells.size(); i++)
    { // for each cell draw triangles with ordered vertex points back to center (points[cell.id]
        if (map.cells[i].vertex.size() == 0) { continue; }
        else { map.cells[i].vertex_offset = offset; }
        sf::Vertex vertices[3];
        sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
        for (int j = 0; j < map.cells[i].vertex.size(); j++)
        {
            vertices[0].color = color;
			vertices[1].color = color;
			vertices[2].color = color;
			vertices[0].position = map.voronoi_points[map.cells[i].vertex[j]];
			vertices[1].position = map.voronoi_points[map.cells[i].vertex[(j + 1) % map.cells[i].vertex.size()]];
			vertices[2].position = map.points[map.cells[i].id];
			vertexBuffer.update(vertices, 3, offset);
			offset += 3;
		}
    }


    sf::Vector2f oldPos;
    bool moving = false;

    float zoom = 1;
    // Retrieve the window's default view
    sf::View view = window.getDefaultView();

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
                    std::size_t cellIndex = map.getCellIndex(mousePos);
                    if(cellIndex != vor::INVALID_INDEX) {
                        std::cout << "ID: " << cellIndex << " Height: " << map.cells[cellIndex].height << " riverBool: " << map.cells[cellIndex].riverBool << " oceanBool: " << map.cells[cellIndex].oceanBool << " snowBool: " << map.cells[cellIndex].snowBool << " lakeBool: " << map.cells[cellIndex].lakeBool << std::endl;
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
				if (event.key.code == sf::Keyboard::Escape)
				    window.close();
                else if (event.key.code == sf::Keyboard::A)
                { // paint the cells triangles in vertexBuffer
                    
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    
                    std::size_t cellIndex = map.getCellIndex(mousePos);
                    unsigned int offset = map.cells[cellIndex].vertex_offset;
                    int n_vertices = map.cells[cellIndex].vertex.size();

                    // now change color on vertex
                    sf::Color color(0, 0, 255, 255);
                    for (int i = 0; i < n_vertices * 3; i++)
                    {
                        continue;
                    }
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
        
        window.draw(vertexBuffer);

        window.display();
    }

    return 0;
}

