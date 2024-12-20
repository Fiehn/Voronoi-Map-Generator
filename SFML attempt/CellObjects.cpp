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
	// For each cell, get a random point within the cell
	// if the cell is a repeat of an already assigned cell, set the point to the already assigned point
	// if the cell is a new cell, get a random point within the cell
	// then copy and reverse the path to make it a loop
    // then calculate the length of the path

    path.reserve(cells.size() * 2);

	// Create a map to store if a cell has already been assigned a point
	std::map<std::size_t, std::size_t> assignedPoints;

    for (size_t i = 0; i < cells.size(); i++) {
		if (assignedPoints.find(cells[i]) != assignedPoints.end()) {
			path.push_back(path[assignedPoints[cells[i]]]);
		}
		else {
			sf::Vector2f point = points[cells[i]];
			path.push_back(point);
			assignedPoints[cells[i]] = i;
		}
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