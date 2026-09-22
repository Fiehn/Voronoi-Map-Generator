# Voronoi-Map-Generator
A procedural map generator using C++ SFML. The map is made up of a Voronoi diagram of cells.
The project was inspired by [Azgaar's fantasy map generator](azgaar.github.io) and [Undiscovered Worlds](https://undiscoveredworlds.blogspot.com/)
## Voronoi
Code for the Delaunay triangulation was adapted to work with SFML from [delfrrr's implementation of the algorithm](https://github.com/delfrrr/delaunator-cpp). 
The GUI is created using [Dear ImGui](https://github.com/ocornut/imgui?tab=readme-ov-file#dear-imgui) with the seperate [SFML backend](https://github.com/SFML/imgui-sfml). 
[SFML](https://github.com/SFML/SFML) is used as the graphics and event engine.
Furthermore a few utilities libraries are used: 
[JSON for modern C++](https://github.com/nlohmann/json)
[ImGuiFD](https://github.com/Julianiolo/ImGuiFD) with a few backend changes
[ImPlot](https://github.com/epezent/implot) is used for the plots in GUI
All used under the MIT lisence.

## Features
* Voronoi Diagram with SFML
* Jittered grid point generation
* Hybrid K-Point and continent Height generation, noising and smoothing
* River generation, through tree search
* Wind speed and direction for each cell (Based on convergence lines, and then randomly assigns strength and direction based on region. Then averaging to smooth)
* Expandable custom biome generation using GMM (The biomes will be unnamed and randomly generated)
* UI with map switching, and new map creation.
* Resources at strategic locations
* Atmosphere composition and planet sizing
* CPU and GPU vertex generation and switching
* Prototype: Population simulation mechanics including culture generation and religion generator with graph grammar rules for deity generation.
<img width="2497" height="1500" alt="image" src="https://github.com/user-attachments/assets/4aa88d6d-64f9-478b-8312-1f3bf505f5e7" />
<img width="2488" height="1492" alt="image" src="https://github.com/user-attachments/assets/8ae7512b-548f-44af-a014-065252cefb82" />

## Generation
* Rivers / Lakes
* Percipitation
* Temperature
* Height
* Resources
* Wind is based on the grids that are already used for spatial hashing, thus it will scale with that implementation. It needs to account for ocean cells and potentially height of other cells. 
* Biomes are generated based on the previous factors using GMM clustering on a default 10 clusters, then the average of the neighbors biome probabilities are added with a decay factor to and biomes are decided by highest probability. These clusters then represent biomes that need to be named. Oceans also have biomes (clearly seperated in clustering). This is can lead to some tricky biome generation if there is a lot of variation in the ocean cells, often an indication that there are not enough biomes at generation time, so regening biomes with more clusters is often the way to go. Colors are random and each represent a biome.
<img width="2493" height="1504" alt="image" src="https://github.com/user-attachments/assets/567c0c6e-ef29-44cf-97d7-51e6a8c03a8f" />

