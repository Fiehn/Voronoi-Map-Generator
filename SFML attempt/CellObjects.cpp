#pragma once
#include "CellObjects.hpp"
#include <algorithm>
#include <stack>

void Biome::setValues(const std::map<std::string, float>& value) {
    values = value;
}


River::River(int id) {
	this->id = id;
};

void River::purgeRiver(std::vector<Cell>& map) {
    for (size_t i = 0; i < cells.size(); i++) {
        map[cells[i]].riverBool = false;
    }
    pathCells.clear();
    cells.clear();
    len = 0;
};

static float riverCellScore(const Cell& cell) {
	float score = 1;
	if (cell.riverBool) score += 1;
	if (cell.lakeBool) score += 2;
	if (cell.oceanBool) score += 3;
	return score;
};

void River::calcPathCells(const std::vector<Cell>& map) {
	// Will create something assembling a Minimum Spanning Tree
	// Using a Depth First Search algorithm to find the path
	// Prioritizing cells with rivers, then lakes, then oceans


    pathCells.clear();
    pathCells.reserve(cells.size() * 2); // Reserve enough space for the path

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
    pathCells.push_back(q);

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
            pathCells.push_back(neighbor);
            stack.push(neighbor);
            foundUnvisited = true;
        }

        // If no unvisited neighbors, backtrack
        if (!foundUnvisited) {
            stack.pop();
            if (!stack.empty()) {
                // Add the backtracking point to the path
                pathCells.push_back(stack.top());
            }
        }
    }

    calcLen();
};

void River::addCell(std::size_t cell) {
	cells.push_back(cell);
};

Continent::Continent(int id) {
	this->id = id;
};

void Continent::calcCenterOnSphere(const std::vector<glm::vec3>& points)
{
	glm::vec3 sum(0.f);
	for (std::size_t cell : cells) {
		sum += points[cell];
	}
	// A plate covering the whole sphere evenly has no meaningful centre, keep the old one
	if (glm::length(sum) > 1e-6f) {
		centerDir = glm::normalize(sum);
	}
};

void Continent::addBoundryCell(std::size_t cellId)
{
    boundCells.push_back(cellId);
};
