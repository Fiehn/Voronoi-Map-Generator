# Voronoi-Map-Generator
c++ SFML implementation of a Voronoi diagram, for future map generation implementations. I am new at c++ so there will be many mistakes, errors and bad coding practices.
This is my way to learn the language. The project was inspired by Azgaar's fantasy map generator and started out as my attempt to implement a voronoi diagram in c++.
## Voronoi
Code for the Delaunay triangulation was adapted to work with SFML from delfrrr's implementation of the algorithm (https://github.com/delfrrr/delaunator-cpp).
Used under the MIT lisence.
The Voronoi was then created from the dual graph of the triangulation. It is however missing a few conditions of creating diagrams, but it works.

## Features
For now only a few features exist, with the diagram being the main one:
* Voronoi Diagram with SFML
* Jittered grid point generation
* Height generation and smoothing, with custom algorithm (Perlin noise was discarded for being too random)

## Features being worked on
Many more things are needed but this is the current priority list:
* Clean up of code, espacially with regrads to Delaunay
* Typecasting ?
* Bounding boxes for voronoi, not that necesary with the current system, only a few places it does not generate cells which adds to the astetic
* Making zooming and moving the map a thing
* Making UI system to alter the map while the program is running
** Making a new button, seed inserter, altering parameters perhaps with sliders
* New point generation system, Loyd relaxation is an option?
* creating rainfall and percipitation
* average temperature
* wind speed and direction for each cell
* drawing coastline with smooth line
* smooth cell transitions ??? big challenge



