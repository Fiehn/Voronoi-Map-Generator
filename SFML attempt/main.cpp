#include <SFML/Graphics.hpp>
#include <iostream>
#include "delany.hpp"
#include "points.hpp"
#include "cell.hpp"
#include <time.h>
#include <cstdlib>

// For random use:
// rand() << 15 | rand();
// It will shift the first random number 15 bits and then insert another random number in those 15 bits, thereby generating 32 bits


int main()
{
    std::srand(time(NULL));
    int windowWidth = 1800;
    int windowHeight = 1000;

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "SFML");
    window.setFramerateLimit(27); // For now there is no reason to have even this high framerate
    sf::View view = window.getDefaultView();
    
    // as a hash table????? would be better, perhaps?
    std::vector<sf::Vector2f> points;

    generatePoints(points, 200, 150, windowWidth, windowHeight,10.f);
    //generateCells(points);

    // Make sure that it just returns voronoi and nothing else
    Delaunator d(points);
    std::vector<sf::Vector2f> voronoi_points = voronoi(d,points);

    // This will be the initialization of the map, with height and other things
    // Very slow, took 30 sec with 1000, 600 granted that is overkill but still, should be optimized or something
    for (int i = 0; i < map.size(); i++) {
        if (map[i].vertex.size() == 0) { continue; };
        map[i].bubble_sort_angles(points,voronoi_points);
        //map[i].height = perlin(points[i].x, points[i].y);
    }
    
    random_height_gen(map, 6, 0.04, 0.02, 0.002, 1.0, "Front");

    // Initialization for drawing the map
    sf::RenderTexture bgMap;
    bgMap.create(windowWidth, windowHeight);

    for (int i = 0; i < map.size(); i++) {
        int water = 1;
        if (map[i].height < 0.50) { water = 0; }
        sf::Color color(255*water, 255*water, 255*(1-water), 255 * map[i].height); // Here it loses data in conversion which is fine but hives warning so ask it to do it
        if (map[i].vertex.size() == 0) { continue; };
        sf::VertexArray T(sf::TriangleFan, map[i].vertex.size() + 1);

        for (int j = 0; j < map[i].vertex.size(); j++) {
            T[j].position = voronoi_points[map[i].vertex[j]];
            T[j].color = color;

        }

        T[map[i].vertex.size()].position = voronoi_points[map[i].vertex[0]];
        T[map[i].vertex.size()].color = color;
        
        bgMap.draw(T);
    }
    
    bgMap.display();
    sf::Sprite background(bgMap.getTexture());

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {            
            if (event.type == sf::Event::Closed)
                window.close();

            /*if (event.type == sf::Event::Resized)
            {
                sf::FloatRect view(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(view));
            }*/
            
            // https://stackoverflow.com/questions/65458106/c-sfml-how-to-adjust-game-view-via-mouse-movements
            if (event.type == sf::Event::MouseButtonPressed)
            {
                view.zoom(0.9f);
                window.setView(view);
            }
            /*
            if (event.type == sf::Keyboard::A)
            {
                view.zoom(1.f);
                window.setView(window.getDefaultView());
            }
            if (event.type == sf::Event::KeyPressed)
            {
                view.move(100.f, 100.f);
            }*/
        }

        
        window.clear();
        
        window.draw(background);

        window.display();        
    }

    return 0;
}

