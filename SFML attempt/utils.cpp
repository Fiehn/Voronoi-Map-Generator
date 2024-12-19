#pragma once
#include "util.hpp"


sf::Color randomColor() {
	ColorTable colorTable;
	return colorTable.getRandomColor();
}

std::vector<sf::Color> randomColors(int numColors) {
	ColorTable colorTable;
	return colorTable.getRandomColors(numColors);
}

std::string colorName(sf::Color color) {
	ColorTable colorTable;
	return colorTable.getName(color);
}

std::string closestColorName(sf::Color color) {
	ColorTable colorTable;
	return colorTable.getClosestName(color);
}

sf::Color colorByName(std::string name) {
	ColorTable colorTable;
	return colorTable.getColor(name);
}


void insert_unique(std::vector<int>& vec, const int& key) {
	if (std::find(vec.begin(), vec.end(), key) == vec.end()) {
		vec.push_back(key);
	}
}
void insert_unique(std::vector<float>& vec, const float& key) {
	if (std::find(vec.begin(), vec.end(), key) == vec.end()) {
		vec.push_back(key);
	}
}
void insert_unique(std::vector<std::size_t>& vec, const std::size_t& key) {
	if (std::find(vec.begin(), vec.end(), key) == vec.end()) {
		vec.push_back(key);
	}
}

sf::Vector2f randomGradient() {
	sf::Vector2f v;

	int randomSignx = rand() % 2;
	int randomSigny = rand() % 2;
	float randomx = (float)rand() / (float)RAND_MAX;
	float randomy = (float)rand() / (float)RAND_MAX;
	v.x = (randomSignx)*randomx + (randomSignx - 1) * randomx;
	v.y = (randomSigny)*randomy + (randomSigny - 1) * randomy;
	return v;
}