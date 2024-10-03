#pragma once
#include <SFML/Graphics.hpp>

class VertexMap {
public:
	sf::VertexBuffer vertexBuffer = sf::VertexBuffer(sf::Triangles, sf::VertexBuffer::Dynamic);
	sf::VertexArray vertexArray = sf::VertexArray(sf::Triangles, 0);
	bool useVertexBuffer;

	VertexMap() {
		useVertexBuffer = sf::VertexBuffer::isAvailable();
	}
	void genVertexMap(vor::Voronoi& map) {
		if (useVertexBuffer) {
			for (std::size_t i = 0; i < map.cells.size(); i++) {
				sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
				for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
					map.vertices[j].color = color;
				}
			}
			vertexBuffer.update(map.vertices.data());
		}
		else {
			for (std::size_t i = 0; i < map.cells.size(); i++) {
				sf::Color color((128 * (1 - map.cells[i].oceanBool)), (255 * (1 - map.cells[i].oceanBool)), 255 / 3 * (map.cells[i].oceanBool + (2 - map.cells[i].riverBool - map.cells[i].lakeBool)), 55 + (sf::Uint8)std::abs(std::ceil(200 * map.cells[i].height)));
				for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
					map.vertices[j].color = color;
				}
			}
			// Fill vertex array / draw triangles from map.vertices
			for (std::size_t i = 0; i < map.vertices.size(); i++) {
				vertexArray.append(map.vertices[i]);
			}
			
		}
	}
	void draw(sf::RenderWindow& window) {
		if (useVertexBuffer) {
			window.draw(vertexBuffer);
		}
		else {
			window.draw(vertexArray);
		}
	}
	void clear() {
		if (useVertexBuffer) {
			vertexBuffer.create(0);
		}
		else {
			vertexArray.clear();
		}
	}
	void create(vor::Voronoi& map) {
		if (useVertexBuffer) {
			vertexBuffer.create(map.vertexCount * 3);
		}
		else {
			vertexArray.resize(map.vertexCount * 3);
		}
	}
	void update(vor::Voronoi& map) {
		if (useVertexBuffer) {
			vertexBuffer.update(map.vertices.data());
		}
		else {
			vertexArray.clear();
			for (std::size_t i = 0; i < map.vertices.size(); i++) {
				vertexArray.append(map.vertices[i]);
			}
		}
	}

	void switchOrigin(vor::Voronoi& map) {
		if (!sf::VertexBuffer::isAvailable()) { return; }

		if (useVertexBuffer) {
			useVertexBuffer = false;
			vertexBuffer.create(0);
			for (std::size_t i = 0; i < map.vertices.size(); i++) {
				vertexArray.append(map.vertices[i]);
			}
		}
		else {
			useVertexBuffer = true;
			vertexArray.clear();
			vertexBuffer.create(map.vertexCount * 3);
			vertexBuffer.update(map.vertices.data());
		}
	}
};

