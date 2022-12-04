#include <SFML/Graphics.hpp>
#include <iostream>
#include "delany.hpp"
#include "points.hpp"
#include "cell.hpp"
#include <time.h>
#include <cstdlib>


// Ok, lets start from the very beginning
// http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/
// https://github.com/Sushisource/DnDTG



int main()
{
    //std::srand(time(NULL));

    sf::RenderWindow window(sf::VideoMode(1400, 800), "SFML");



    generatePoints(points, 200, 150, 1400, 800,10.f);
    //generateCells(points);

    Delaunator d(points);
    voroi(d);
    // With optimization make sure that it just returns voroi and nothing else ;)
    // Also it uses over 10% cpu right now because it is constantly forced to redraw all trianglefans, try making a constant?

    // This will be the initialization of the map, with height and shit
    for (int i = 0; i < map.size(); i++) {
        if (map[i].vertex.size() == 0) { continue; };
        map[i].bubble_sort_angles(points,voroi_points);
        map[i].height = perlin(points[i].x, points[i].y);
    }
    
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {            
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        for (int i = 0; i < map.size(); i++) {
            sf::Color color(255, 255, 255,255/2* map[i].height);
            if (map[i].vertex.size() == 0) { continue; };
            sf::VertexArray T(sf::TriangleFan, map[i].vertex.size() + 1);
            
            for (int j = 0; j < map[i].vertex.size(); j++) {
                T[j].position = voroi_points[map[i].vertex[j]];
                T[j].color = color;

                /*sf::Vertex point(voroi_points[map[i].vertex[j]], sf::Color::White);
                window.draw(&point, 1, sf::Points);*/
            }

            T[map[i].vertex.size()].position = voroi_points[map[i].vertex[0]];
            T[map[i].vertex.size()].color = color;

            window.draw(T);
        }

        window.display();        
    }

    return 0;
}

