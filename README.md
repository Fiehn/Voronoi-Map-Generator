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
* Perlin noise

## Features being worked on
Many more things are needed but this is the current priority list:
* Clean up of code, espacially with regrads to Delaunay and global variables (evil)
* New point generation system, Loyd relaxation is an option?
* Performance with drawing the cells constantly, perhaps moving the drawing into a more static environment somehow??
* Typecasting ?
* Bounding boxes for voronoi
* New height map generation, Perlin noise is limited and can at times feel unnatural
* Making the map not scale with window size but be constant and making zooming a thing



