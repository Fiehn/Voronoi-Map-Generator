# Voronoi-Map-Generator
A C++ SFML implementation of a Voronoi diagram, for future map generation implementations. 
The project was inspired by [Azgaar's fantasy map generator](azgaar.github.io) and to a certain degree [Undiscovered Worlds](https://undiscoveredworlds.blogspot.com/)
## Voronoi
Code for the Delaunay triangulation was adapted to work with SFML from [delfrrr's implementation of the algorithm](https://github.com/delfrrr/delaunator-cpp). The GUI is created using [Dear ImGui](https://github.com/ocornut/imgui?tab=readme-ov-file#dear-imgui) with the seperate [SFML backend](https://github.com/SFML/imgui-sfml). Using [DearImGui](https://github.com/ocornut/imgui) for UI.
All used under the MIT lisence.

## Features
* Voronoi Diagram with SFML
* Jittered grid point generation
* Height generation, noising and smoothing
* River generation (Lacking)
* Wind speed and direction for each cell (Based on convergence lines, and then randomly assigns strength and direction based on region. Then averaging to smooth)
* Expandable custom biome generation using k-means clustering or GMM (The biomes will be unnamed and randomly generated so there will be a need to create a naming system.)
* UI with map switching, and new map creation.
![image](https://github.com/user-attachments/assets/c8fc125f-be00-4915-a3c0-89939533d380)

## Features being worked on

### General
* Delaunay Cleanup
* Type uniformity
* Bounding boxes for Voronoi

### UI
* There is a need for inputting custom values in specific cells aka drawing biomes, height and percepitation yourself
* Regenerate all aspects of the map with custom values.
* Naming Biomes

### External Interactions
* Save and load a map
* draw map based on a txt/json file

### Graphical and Technical upgrades
* Drawing smooth lines for indicating coasts, rivers and lakes

### Generation fixes
* Rivers/Lakes
* Percipitation
* Temperature
* Height (Needs different methods)

## Generation
* Rivers / Lakes
* Rainfall and percipitation
* Temperature

* Wind is based on the grids that are already used for spatial hashing, thus it will scale with that implementation. It needs to account for ocean cells and potentially height of other cells. 
![image](https://github.com/user-attachments/assets/47cda1fe-793b-4175-a792-3319f3b14b78)

* Biomes are generated based on the previous factors using GMM clustering on a default 10 clusters, then the average of the neighbors biome probabilities are added with a decay factor to and biomes are decided by highest probability. These clusters then represent biomes that need to be named. Oceans also have biomes (clearly seperated in clustering). This is can lead to some tricky biome generation if there is a lot of variation in the ocean cells, often an indication that there are not enough biomes at generation time, so regening biomes with more clusters is often the way to go. Colors are random and each represent a biome.
![image](https://github.com/user-attachments/assets/c7670b81-752a-458b-abaa-fe9ce667604f)
