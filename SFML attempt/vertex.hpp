#pragma once
#include <SFML/Graphics.hpp>
#include "BoundaryRender.hpp"

class VertexMap {
public:
	sf::VertexBuffer vertexBuffer = sf::VertexBuffer(sf::Triangles, sf::VertexBuffer::Dynamic);
	sf::VertexArray vertexArray = sf::VertexArray(sf::Triangles, 0);
	bool useVertexBuffer;

	// Smoothed boundaries, map.vertices holds the cell colours and the renderer builds the mesh that is drawn
	BoundaryRenderer boundaries;
	const RenderSettings* renderSettings = nullptr; // nullptr draws map.vertices directly
	int mapMode = 0; // Current mapType, decides which boundary types are smoothed

	VertexMap() {
		useVertexBuffer = sf::VertexBuffer::isAvailable();
	}
	// Value map colours, the defaults when there are no render settings
	const MapColors& colors() const {
		static const MapColors defaults;
		return renderSettings ? renderSettings->colors : defaults;
	}
	void genVertexMap(vor::Voronoi& map) {
		for (std::size_t i = 0; i < map.cells.size(); i++) {
			sf::Color color = heightColor(colors().height, map.cells[i].height, map.cells[i].oceanBool);
			for (size_t j = map.cells[i].vertex_offset; j < map.cells[i].vertex_offset + map.cells[i].vertex.size() * 3; j++) {
				map.vertices[j].color = color;
			}
		}
		update(map);
	}
	void draw(sf::RenderWindow& window) {
		if (useVertexBuffer) {
			window.draw(vertexBuffer);
		}
		else {
			window.draw(vertexArray);
		}
		if (renderSettings) {
			boundaries.draw(window, *renderSettings);
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
		boundaries.invalidate();
		if (useVertexBuffer) {
			vertexBuffer.create(map.vertexCount * 3);
		}
		else {
			vertexArray.resize(map.vertexCount * 3);
		}
	}
	void update(vor::Voronoi& map) {
		const std::vector<sf::Vertex>& vertices = renderSettings ? boundaries.sync(map, *renderSettings, mapMode) : map.vertices;
		if (useVertexBuffer) {
			// The smoothed mesh has more vertices than map.vertices and its size changes with the settings
			if (vertexBuffer.getVertexCount() != vertices.size()) {
				vertexBuffer.create(vertices.size());
			}
			vertexBuffer.update(vertices.data());
		}
		else {
			vertexArray.resize(vertices.size());
			for (std::size_t i = 0; i < vertices.size(); i++) {
				vertexArray[i] = vertices[i];
			}
		}
	}

	void switchOrigin(vor::Voronoi& map) {
		if (!sf::VertexBuffer::isAvailable()) { return; }

		if (useVertexBuffer) {
			useVertexBuffer = false;
			vertexBuffer.create(0);
		}
		else {
			useVertexBuffer = true;
			vertexArray.clear();
		}
		update(map);
	}
};
