#pragma once
#include <vector>
#include <string>
#include <SFML/System/Vector2.hpp>


class Biome {
public:
	int vegetationType = 0;
};

class River {
public:
	float len = 0;
	float maxWidth = 0;
	float maxDepth = 0;
	float flowSpeed = 0;
	std::vector<int> cells;
};







