#pragma once
#include "CellObjects.hpp"
#include "util.hpp"

River::River(int id) {
	this->id = id;
};

void River::purgeRiver(std::vector<Cell>& map) {
    for (size_t i = 0; i < cells.size(); i++) {
        map[cells[i]].riverBool = false;
    }
    path.clear();
    cells.clear();
    len = 0;
};

sf::VertexArray River::drawRiver() {
    sf::VertexArray river(sf::LinesStrip, path.size());
    for (size_t i = 0; i < path.size(); i++) {
        river[i].position = path[i];
        river[i].color = sf::Color::Blue;
    }
    return river;
};

void River::calcPath(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& points) {
	// Pretend to be a graph.
    // Create a path from the starting cell to the end cell by follwoing the neighbors of the current cell
	// the end cell is the same as the starting cell so the path will loop back to the start
    // all cells needs at least 1 visit

    path.reserve(cells.size());

	std::size_t q = 0;

	// Find the starting cell by looking at any adjacent cell that is ocean or lake:
    for (std::size_t i = 0; i < cells.size(); i++) {
        std::size_t startCell = cells[i];
        for (std::size_t i = 0; i < map[startCell].neighbors.size(); i++) {
            if (map[map[startCell].neighbors[i]].lakeBool || map[map[startCell].neighbors[i]].oceanBool) {
                endCell = map[startCell].neighbors[i];
                break;
            }
        }
		if (endCell != 0) {
			break;
		}
    }

	if (endCell == 0) {
		// find a neighboring cell that is not a part of the river but is a river cell
        for (std::size_t i = 0; i < cells.size(); i++) {
			std::size_t startCell = cells[i];
			for (std::size_t i = 0; i < map[startCell].neighbors.size(); i++) {
				if (!map[map[startCell].neighbors[i]].riverBool && map[map[startCell].neighbors[i]].riverBool) {
					endCell = map[startCell].neighbors[i];
					break;
				}
			}
			if (endCell != 0) {
				break;
			}
        }
    }
	// Start the path from the end cell
	q = endCell;
	path.push_back(points[q]);
	cells.push_back(endCell);

    // This is amount of times visited for each cell based on its index in the cells vector
	std::vector<int> visited(cells.size(), 0);
    
	// find index of q in cells
	std::size_t index = std::find(cells.begin(), cells.end(), q) - cells.begin();
	visited[index] = 1;

	int count = 0;
    while (true) {
        if (count > 1000) {
			std::cout << "Error: River pathfinding took too long" << std::endl;
            break;
        }
		if (q == endCell && count > 0) {
			break;
		}

		std::size_t temp = q;
		std::size_t temp_idx = index;
		for (size_t i = 0; i < map[q].neighbors.size(); i++) {
			std::size_t n = map[q].neighbors[i];
			std::size_t index = std::find(cells.begin(), cells.end(), n) - cells.begin();
            if (index < cells.size() && visited[index] <= visited[temp_idx]) {
				temp = n;
				temp_idx = index;
			}
		}
		if (temp == q) {
			//std::cout << "Error: River pathfinding failed for river " << id << std::endl;
			break;
		}

        visited[temp_idx] += 1;
        q = temp;
        path.push_back(points[temp]);
		count += 1;
    }
    
    calcLen();
	std::vector<sf::Vector2f> reversedPath = path;
	std::reverse(reversedPath.begin(), reversedPath.end());
	path.insert(path.end(), reversedPath.begin(), reversedPath.end());
};

void River::addCell(std::size_t cell) {
	cells.push_back(cell);
};

void Biome::setValues(const std::map<std::string, float>& value) {
    values = value;

    if (values.at("Ocean") > 0.001f) {
        isOcean = true;
        values.at("Ocean") = 1;
    }
    else {
        isOcean = false;
        values.at("Ocean") = 0;
    }
}