# Voronoi-Map-Generator
A C++ SFML implementation of a Voronoi diagram, for future map generation implementations. 
The project was inspired by Azgaar's fantasy map generator (azgaar.github.io).
## Voronoi
Code for the Delaunay triangulation was adapted to work with SFML from delfrrr's implementation of the algorithm (https://github.com/delfrrr/delaunator-cpp).
Used under the MIT lisence.
The Voronoi was then created from the dual graph of the triangulation.

## Features
* Voronoi Diagram with SFML
* Jittered grid point generation
* Height generation, noising and smoothing
* River generation (Lacking)
* Wind speed and direction for each cell (Based on convergence lines, and then randomly assigns strength and direction based on region. Then averaging to smooth)
![image](https://github.com/Fiehn/Voronoi-Map-Generator/assets/81577064/3fbc5b37-b68e-408e-9fb7-a7658099e2dd)
## Features being worked on

### VertexBuffer
* Threads to work better with VertexBuffer and be cleaner
* Create functions for changes in and creation of VertexBuffer
* VertexArray implementation for lack of GPU

### General
* Delaunay Cleanup
* Type uniformity
* Bounding boxes for Voronoi

### UI
* UI system to alter the map while the program is running
* Making a new button, seed inserter, altering parameters perhaps with sliders

### Generation
* Rivers / Lakes
* Rainfall and percipitation
* Temperature

* Wind is based on the grids that are already used for spatial hashing, thus it will scale with that implementation. It needs to account for ocean cells and potentially height of other cells. 
![image](https://github.com/Fiehn/Voronoi-Map-Generator/assets/81577064/c4b033b6-1c4b-436e-8c2c-1ad8a04636d2)

### Graphical and Technical upgrades
* Drawing smooth lines (Potentialy shaders)

