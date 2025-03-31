#pragma once
#include "CellObjects.hpp"
#include "util.hpp"
#include <algorithm>
#include <stack>

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

static float riverCellScore(const Cell& cell) {
	float score = 1;
	if (cell.riverBool) score += 1;
	if (cell.lakeBool) score += 2;
	if (cell.oceanBool) score += 3;
	return score;
};

void River::calcPath(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& points) {
	// Will create something assembling a Minimum Spanning Tree
	// Using a Depth First Search algorithm to find the path
	// Prioritizing cells with rivers, then lakes, then oceans


    path.clear();
    path.reserve(cells.size() * 2); // Reserve enough space for the path

    // Find the starting cell (endCell)
	// Check if any of the neighboring cells are a lake or ocean
    endCell = 0;
    for (std::size_t i = 0; i < cells.size(); i++) {
        std::size_t startCell = cells[i];
        for (std::size_t j = 0; j < map[startCell].neighbors.size(); j++) {
            std::size_t neighbor = map[startCell].neighbors[j];
            if (map[neighbor].lakeBool || map[neighbor].oceanBool) {
                endCell = neighbor;
                break;
            }
        }
        if (endCell != 0) break;
    }

	// If no lake or ocean cells are found, check for river cells
    if (endCell == 0) {
        for (std::size_t i = 0; i < cells.size(); i++) {
            std::size_t startCell = cells[i];
            for (std::size_t j = 0; j < map[startCell].neighbors.size(); j++) {
                std::size_t neighbor = map[startCell].neighbors[j];
                if (map[neighbor].riverId != id && map[neighbor].riverBool) {
                    endCell = neighbor;
                    break;
                }
            }
            if (endCell != 0) break;
        }
    }

    if (endCell == 0) {
        endCell = cells[0];
    }

    // Start the path from the end cell
    std::size_t q = endCell;
    path.push_back(points[q]);

    // Track visited cells to avoid revisiting
    std::vector<bool> visited(map.size(), false);
    visited[q] = true;

    // Use a stack to keep track of the current path
    std::stack<std::size_t> stack;
    stack.push(q);

    while (!stack.empty()) {
        std::size_t current = stack.top();
        bool foundUnvisited = false;

		// Get all unvisited neighbors
		std::vector<std::size_t> unvisitedNeighbors;
        for (std::size_t i = 0; i < map[current].neighbors.size(); i++) {
            std::size_t neighbor = map[current].neighbors[i];

            // Check if this cell has been visited
            if (!visited[neighbor] && std::find(cells.begin(), cells.end(), neighbor) != cells.end()) {
				unvisitedNeighbors.push_back(neighbor);
            }
        }
        // Sort unvisited neighbors by priority (riverCellScore)
        std::sort(unvisitedNeighbors.begin(), unvisitedNeighbors.end(), [&](std::size_t a, std::size_t b) {
            return riverCellScore(map[a]) > riverCellScore(map[b]); // Higher score = higher priority
            });

        // Visit the highest-priority neighbor
        if (!unvisitedNeighbors.empty()) {
            std::size_t neighbor = unvisitedNeighbors[0]; // Highest-priority neighbor
            visited[neighbor] = true;
            path.push_back(points[neighbor]);
            stack.push(neighbor);
            foundUnvisited = true;
        }

        // If no unvisited neighbors, backtrack
        if (!foundUnvisited) {
            stack.pop();
            if (!stack.empty()) {
                // Add the backtracking point to the path
                path.push_back(points[stack.top()]);
            }
        }
    }

    calcLen();
};

void River::addCell(std::size_t cell) {
	cells.push_back(cell);
};

void Lake::calcArea() {
	area = polygonArea(bounds);
};

void Lake::calcMaxDepth() {

};

void Lake::calcBounds(const std::vector<sf::Vector2f>& voronoi_points, const std::vector<Cell>& map) {
	bounds.clear();
	bounds.reserve(cells.size() * 2);
	// We need to find the outer bounds of the lake cells
	// We can do this by finding the convex hull of the lake cells
	// Using Chan's algorithm

	// First get all the points of the lake
	std::vector<sf::Vector2f> points;
	points.reserve(cells.size() * 10);
	// points are stored in map[cells[i]].vertex
	for (std::size_t i = 0; i < cells.size(); i++) {
		for (std::size_t j = 0; j < map[cells[i]].vertex.size(); j++) {
			points.push_back(voronoi_points[map[cells[i]].vertex[j]]);
		}
	}

	// Find the convex hull of the points
	bounds = convexHull(points); /// THIS IS NOT THE WAY YOU dimbo
};

sf::VertexArray Lake::drawLake() { /// THis is not working correctly fix it
	sf::VertexArray lake(sf::TriangleStrip, bounds.size()+1);
	// Set the center of the lake
	//lake[0].position = computeCentroid(bounds);
	//lake[0].color = sf::Color::Blue;
	// Set the bounds of the lake
	for (size_t i = 0; i < bounds.size(); i++) {
		lake[i].position = bounds[i];
		lake[i].color = sf::Color::Blue;
	}
	return lake;
};

Continent::Continent(int id) {
	this->id = id;
};

void Continent::calcCenter(const std::vector<sf::Vector2f>& voronoi_points, const std::vector<Cell>& map) {
	// Find the center of the continent
	sf::Vector2f sum = sf::Vector2f(0, 0);
	for (std::size_t i = 0; i < cells.size(); i++) {
		sum += voronoi_points[map[cells[i]].vertex[0]];
	}
	center = sum / static_cast<float>(cells.size());
};

void Continent::finishContinent(const std::vector<sf::Vector2f>& voronoi_points, const std::vector<Cell>& map)
{
	calcCenter(voronoi_points, map);
};

void Continent::addBoundryCell(std::size_t cellId)
{
    boundCells.push_back(cellId);
}