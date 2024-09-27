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
* Expandable custom biome generation using k-means clustering (The biomes will be unnamed and randomly generated so there will be a need to create a naming system.)
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

* Biomes are generated based on the previous factors using k-means clustering on a default 10 clusters, these clusters then represent biomes that need to be named. Oceans can also have biomes. This is can lead to some tricky biome generation if there is a lot of variation in the ocean cells. Colors are random and each represent a biome.
![image](https://github.com/user-attachments/assets/29e6e047-8942-465b-9d67-5cfc5a398ce0)

### Graphical and Technical upgrades
* Drawing smooth lines (Potentialy shaders)

