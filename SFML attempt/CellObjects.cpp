#pragma once
#include "CellObjects.hpp"
#include "util.hpp"

River::River(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& voronoi_points, std::vector<std::size_t> cells) {
	this->cells = cells;
    calcPath(map, voronoi_points);
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

void River::calcPath(const std::vector<Cell>& map, const std::vector<sf::Vector2f>& voronoi_points) {
    // Get a random point within the cell by vertices
    path.reserve(cells.size());

    for (size_t i = 0; i < cells.size(); i++) {
        std::vector<sf::Vector2f> vertices;
        for (size_t j = 0; j < map[cells[i]].vertex.size(); j++) {
            vertices.push_back(voronoi_points[map[cells[i]].vertex[j]]);
        }
        sf::Vector2f point = RandomPointInPolygon(vertices);
        path.push_back(point);
    }
    calcLen();
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