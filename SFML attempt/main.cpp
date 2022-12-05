#include <SFML/Graphics.hpp>
#include <iostream>
#include "delany.hpp"
#include "points.hpp"
#include "cell.hpp"
#include <time.h>
#include <cstdlib>


int main()
{
    //std::srand(time(NULL));
    int windowWidth = 1400;
    int windowHeight = 800;

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "SFML");

    // as a hash table????? would be better perhaps?
    std::vector<sf::Vector2f> points;

    generatePoints(points, 300, 100, windowWidth, windowHeight,10.f);
    //generateCells(points);

    // With optimization make sure that it just returns voroi and nothing else ;)
    // kinda important!
    Delaunator d(points);
    std::vector<sf::Vector2f> voronoi_points = voronoi(d,points);

    // This will be the initialization of the map, with height and shit
    // Very slow, took 30 sec with 1000, 600 granted that is overkill but still, should be optimized or something
    for (int i = 0; i < map.size(); i++) {
        if (map[i].vertex.size() == 0) { continue; };
        map[i].bubble_sort_angles(points,voronoi_points);
        map[i].height = perlin(points[i].x, points[i].y);
    }
    
    // Initialization for drawing the map
    sf::RenderTexture bgMap;
    bgMap.create(windowWidth, windowHeight);

    for (int i = 0; i < map.size(); i++) {
        sf::Color color(255, 255, 255, 255 / 2 * map[i].height);
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
        }

        window.clear();
        
        window.draw(background);

        window.display();        
    }

    return 0;
}

