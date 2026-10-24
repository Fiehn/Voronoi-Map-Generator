#pragma once
#include "CellObjects.hpp"
#include <algorithm>
#include <stack>

void Biome::setValues(const std::map<std::string, float>& value) {
    values = value;
}


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
