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

sf::Color closeRandomColorChange(sf::Color color)
{
	color.r += RandomBetween(-10, 10);
	color.b += RandomBetween(-10, 10);
	color.g += RandomBetween(-10, 10);
	return color;
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

std::vector<sf::Vector2f> convexHull(const std::vector<sf::Vector2f>& points)
{
	// Andrew's Monotone Chain algorithm
	if (points.size() <= 3) return points;

	// Create a copy and sort the points lexographically (x then y)
	std::vector<sf::Vector2f> sortedPoints = points;
	std::sort(sortedPoints.begin(), sortedPoints.end(), [](const sf::Vector2f& a, const sf::Vector2f& b) {
		return a.x < b.x || (a.x == b.x && a.y < b.y);
		});

	std::vector<sf::Vector2f> hull;
	hull.reserve(2 * sortedPoints.size()); // Preallocate memory

	// Build the lower hull
	for (const auto& point : sortedPoints) {
		while (hull.size() >= 2) {
			const sf::Vector2f& a = hull[hull.size() - 2];
			const sf::Vector2f& b = hull[hull.size() - 1];
			// Cross product to determine turn direction
			float cross = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
			if (cross <= 0) { // Non-left turn, remove the last point
				hull.pop_back();
			}
			else {
				break;
			}
		}
		hull.push_back(point);
	}

	// Build the upper hull, starting from the second last point to avoid duplication
	size_t lowerEnd = hull.size();
	for (auto it = sortedPoints.rbegin() + 1; it != sortedPoints.rend(); ++it) {
		const auto& point = *it;
		while (hull.size() > lowerEnd) { // Ensure we only modify upper hull points
			const sf::Vector2f& a = hull[hull.size() - 2];
			const sf::Vector2f& b = hull[hull.size() - 1];
			float cross = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
			if (cross <= 0) {
				hull.pop_back();
			}
			else {
				break;
			}
		}
		hull.push_back(point);
	}
	// Remove the last point if it's duplicate of the first
	if (!hull.empty() && hull.front() == hull.back()) {
		hull.pop_back();
	}

	return hull;
}

sf::Vector2f computeCentroid(const std::vector<sf::Vector2f>& points)
{
	sf::Vector2f centroid(0.0f, 0.0f);
	for (const auto& point : points) {
		centroid += point;
	}
	centroid.x /= points.size();
	centroid.y /= points.size();
	return centroid;
}

std::vector<sf::Vector2f> orderPoints(std::vector<sf::Vector2f> points)
{
	if (points.size() < 3) return points;  // Not a polygon

	sf::Vector2f centroid = computeCentroid(points);

	// Sort points by their angle relative to the centroid
	std::sort(points.begin(), points.end(), [&centroid](const sf::Vector2f& a, const sf::Vector2f& b) {
		return computeAngle(centroid, a) < computeAngle(centroid, b);
		});

	return points;
}

float polygonArea(const std::vector<sf::Vector2f>& points)
{
	const size_t n = points.size();
	if (n < 3) return 0.0f;  // Not a polygon

	// Order the points sequentially
	std::vector<sf::Vector2f> orderedPoints = orderPoints(points);

	// Apply the Shoelace formula
	float area = 0.0f;
	for (size_t i = 0; i < n; ++i) {
		const size_t j = (i + 1) % n;  // Next vertex index (wraps around)
		area += (orderedPoints[i].x * orderedPoints[j].y);
		area -= (orderedPoints[i].y * orderedPoints[j].x);
	}

	return std::abs(area) / 2.0f;
}