# Voronoi-Map-Generator
A C++ SFML implementation of a Voronoi diagram, for future map generation implementations. 
The project was inspired by Azgaar's fantasy map generator (azgaar.github.io), bacuse of performance issues in that generator when maps get large enough.
## Voronoi
Code for the Delaunay triangulation was adapted to work with SFML from delfrrr's implementation of the algorithm (https://github.com/delfrrr/delaunator-cpp).
Used under the MIT lisence.
The Voronoi was then created from the dual graph of the triangulation.

## Features
For now only a few features exist, with the diagram being the main one:
* Voronoi Diagram with SFML
* Jittered grid point generation
* Height generation, noising and smoothing
* River generation (weak)

## Features being worked on
Many more things are needed but this is the current priority list:
* Delaunay Cleanup
* Type uniformity
* Bounding boxes for Voronoi
* Altering map through VertexBuffer
* UI system to alter the map while the program is running
** Making a new button, seed inserter, altering parameters perhaps with sliders
* Rainfall and percipitation
* Temperature
* Wind speed and direction for each cell
* Drawing smooth lines
** Potentialy shaders
