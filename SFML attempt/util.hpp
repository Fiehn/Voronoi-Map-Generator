#ifndef UTIL_HPP
#define UTIL_HPP

#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <numeric>
#include <string>
#include <map>
#include <random>
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <stdexcept>


// I need to create a Random engine class that can be used to generate random numbers


inline long rand_long() // Should only be used in the case that there is a need for larger variables
{
    return rand() << 15 | rand();
}

// long RAND_LONG_MAX = RAND_MAX << 15 | RAND_MAX;

inline float clamp(float x, float max, float min) 
{
	// Check if min is greater than max
    if (min > max) {
        std::swap(min, max);
    }
    if (x < min) { return min; }
    if (x > max) { return max; }
    return x;
}
inline int clamp_int(int x, int max, int min)
{
	if (x < min) { return min; }
	if (x > max) { return max; }
	return x;
}

inline sf::Vector2f clampVect2f(sf::Vector2f vect, float maxx, float minx, float maxy, float miny)
{
    sf::Vector2f new_vect = vect;
    if (vect.x < minx) { new_vect.x = minx; }
    if (vect.x > maxx) { new_vect.x = maxy; }
    if (vect.y < miny) { new_vect.y = miny; }
    if (vect.y > maxy) { new_vect.y = maxy; }
    return new_vect;
}

inline double dist(sf::Vector2f a, sf::Vector2f b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

inline float RandomBetween(float smallNumber, float bigNumber)
{
    float diff = bigNumber - smallNumber;
    return (((float)rand() / RAND_MAX) * diff) + smallNumber;
}

inline int RandomBetweenInt(int smallNumber, int bigNumber)
{
    if (smallNumber > bigNumber) {
        std::swap(smallNumber, bigNumber);
    }
    int diff = bigNumber - smallNumber + 1;
    if (diff <= 0) {
        return smallNumber; // Edge case: return the only valid value
    }
    return (rand() % diff) + smallNumber;
}

sf::Vector2f randomGradient();

template <typename T>
inline T pop_front_i(std::vector<T>& v)
{ // has O(n) complexity, for larger vectors use a que
    if (v.empty()) { return 0; }

    T value = v[0];
    v.erase(v.begin());
    return value;
}

template <typename T>
inline T pop_random_i(std::vector<T>& v)
{ // has O(1) complexity
    if (v.empty()) { return 0; }

    size_t rand_index = rand_long() % v.size();
    std::swap(v[rand_index], v.back());
    T value = v.back();
    v.pop_back();
    return value;
}


template <typename T>
class Queue {
public:
    void push(const T& value) {
        m_queue.push_back(value);
    }

    T pop_front() {
        if (empty()) {
            throw std::out_of_range("Queue is empty");
        }
        T value = m_queue[front_index];
        ++front_index;

        if (front_index > m_queue.size() / 2) {
            m_queue.erase(m_queue.begin(), m_queue.begin() + front_index);
            front_index = 0;
        }
        return value;
    }

    bool empty() const {
        return front_index >= m_queue.size();
    }

    size_t size() const {
        return m_queue.size() - front_index;
    }

    T pop_random() {
        if (empty()) {
            throw std::out_of_range("Queue is empty");
        }
        size_t rand_index = front_index + (rand_long() % (m_queue.size() - front_index));
        std::swap(m_queue[rand_index], m_queue.back());
        T value = m_queue.back();
        m_queue.pop_back();
        return value;
    }

private:
    std::vector<T> m_queue;
    size_t front_index = 0;
};

inline float normalized_value(float value, float max, float min) { return fabs((value - min) / (max - min)); }

void insert_unique(std::vector<int>& vec, const int& key);
void insert_unique(std::vector<float>& vec, const float& key);
void insert_unique(std::vector<std::size_t>& vec, const std::size_t& key);


// Kahan and Babuska summation, Neumaier variant; accumulates less FP error
inline double sum_vec_double(const std::vector<double>& x) 
{
    double sum = x[0];
    double err = 0.0;

    for (int i = 1; i < x.size(); i++) {
        const double k = x[i];
        const double m = sum + k;
        err += std::fabs(sum) >= std::fabs(k) ? sum - m + k : k - m + sum;
        sum = m;
    }
    return sum + err;
}
inline float sum_vec_float(const std::vector<float>& x)
{
    float sum = x[0];
    float err = 0.0;

    for (int i = 1; i < x.size(); i++) {
        const float k = x[i];
        const float m = sum + k;
        err += std::fabs(sum) >= std::fabs(k) ? sum - m + k : k - m + sum;
        sum = m;
    }
    return sum + err;
}

inline size_t fast_mod(const size_t i, const size_t c) 
{
    return i >= c ? i % c : i;
}

// monotonically increases with real angle, used for delaunay
inline double pseudo_angle(const double dx, const double dy) 
{
    const double p = dx / (std::abs(dx) + std::abs(dy));
    return (dy > 0.0 ? 3.0 - p : 1.0 + p) / 4.0; // [0..1)
}

inline double circumradius(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c) 
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double ex = c.x - a.x;
    const double ey = c.y - a.y;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = (ey * bl - dy * cl) * 0.5 / d;
    const double y = (dx * cl - ex * bl) * 0.5 / d;

    if ((bl > 0.0 || bl < 0.0) && (cl > 0.0 || cl < 0.0) && (d > 0.0 || d < 0.0)) {
        return x * x + y * y;
    }
    else {
        return std::numeric_limits<double>::max();
    }
}

// 
inline bool orient(sf::Vector2f p, sf::Vector2f q, sf::Vector2f r) 
{
    return (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y) < 0.0;
}

// Return the center of the circle
inline sf::Vector2f circumcenter(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c) 
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double ex = c.x - a.x;
    const double ey = c.y - a.y;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = a.x + (ey * bl - dy * cl) * 0.5 / d;
    const double y = a.y + (dx * cl - ex * bl) * 0.5 / d;

    return sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
}

inline bool in_circle(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f p) {
    const double dx = a.x - p.x;
    const double dy = a.y - p.y;
    const double ex = b.x - p.x;
    const double ey = b.y - p.y;
    const double fx = c.x - p.x;
    const double fy = c.y - p.y;

    const double ap = dx * dx + dy * dy;
    const double bp = ex * ex + ey * ey;
    const double cp = fx * fx + fy * fy;

    return (dx * (ey * cp - bp * fy) -
        dy * (ex * cp - bp * fx) +
        ap * (ex * fy - ey * fx)) < 0.0;
}

constexpr double EPSILON = std::numeric_limits<double>::epsilon();

inline bool check_pts_equal(sf::Vector2f a, sf::Vector2f b) {
    return std::fabs(a.x - b.x) <= EPSILON &&
        std::fabs(a.y - b.y) <= EPSILON;
}

// Compare a list of points to a center point, used for sorting
struct compare_dist_to_point {
    std::vector<sf::Vector2f> const& points;
    sf::Vector2f p;

    bool operator()(std::size_t i, std::size_t j) {
        const double d1 = dist(points[i], p);
        const double d2 = dist(points[j], p);
        const double diff1 = d1 - d2;
        const double diff2 = points[i].x - points[j].x;
        const double diff3 = points[i].y - points[j].y;

        if (diff1 > 0.0 || diff1 < 0.0) {
            return diff1 < 0;
        }
        else if (diff2 > 0.0 || diff2 < 0.0) {
            return diff2 < 0;
        }
        else {
            return diff3 < 0;
        }
    }
};

inline float normalizeAngle(float angleDegrees) {
    while (angleDegrees >= 360.0) {
        angleDegrees -= 360.0;
    }
    while (angleDegrees < 0.0) {
        angleDegrees += 360.0;
    }
    return angleDegrees;
}

#define PI 3.14159265

inline float radians(float degrees) {
	return degrees * (static_cast<float>(PI) / 180.0);
}

inline std::vector<float> scalarMultiplication(const std::vector<float>& vec, float scalar) {
    std::vector<float> result;
    result.reserve(vec.size()); // Reserve space for efficiency

    // Perform scalar multiplication
    for (float value : vec) {
        result.push_back(value * scalar);
    }

    return result;
}

template <typename T>
inline std::vector<T> elementWiseAdd(const std::vector<T>& vec1, const std::vector<T>& vec2) {
    // Add two vectors element-wise
    if (vec1.size() != vec2.size()) {
		throw std::invalid_argument("Vectors must be the same size");
	}
	std::vector<T> result;
	result.reserve(vec1.size());
    for (int i = 0; i < vec1.size(); i++) {
		result.push_back(vec1[i] + vec2[i]);
	}
	return result;
}

inline void booleanMapVector_f(std::vector<float>& vec, const std::vector<bool>& mask, bool flipped=false) {
	// Apply a boolean mask to a vector
    if (vec.size() != mask.size()) {
		throw std::invalid_argument("Vectors must be the same size");
	}
    for (int i = 0; i < vec.size(); i++) {
        if (flipped) {
			vec[i] = mask[i] ? 0 : vec[i];
		}
        else {
			vec[i] = mask[i] ? vec[i] : 0;
		}
    }
}

inline float sumf(const std::vector<float>& vec) {
	float sum = 0;
    for (float value : vec) {
		sum += value;
	}
	return sum;
}

inline int chooseIndexMax(std::vector<float> rates) {
    // get index of max value of rates
    return std::distance(rates.begin(), std::max_element(rates.begin(), rates.end()));
}

inline int chooseIndex(std::vector<float> rates) {
    // choose index based on rates (rates are not probabilities but can be any positive number)
    float sum = sumf(rates);
    float random = (float)rand() / RAND_MAX;

    for (int i = 0; i < rates.size(); i++) {
		random -= rates[i] / sum;
        if (random <= 0) {
			return i;
		}
	}
    return rates.size() - 1;
}

inline float normalDistPDF(float mean, float std)
{
    // Box-Muller transform
	float u1 = (float)rand() / RAND_MAX;
	float u2 = (float)rand() / RAND_MAX;
	return mean + std * sqrt(-2 * log(u1)) * cos(2 * PI * u2);
}

inline float betaDist(float alpha, float beta)
{
    // Beta distribution
	float gamma1 = std::tgammaf(alpha);
	float gamma2 = std::tgammaf(beta);
	return gamma1 / (gamma1 + gamma2);
}

inline bool pointInPolygon(const sf::Vector2f& point, const std::vector<sf::Vector2f>& vertices) {
	// Check if a point is within a polygon
	bool inside = false;
	for (int i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++) {
		if (((vertices[i].y > point.y) != (vertices[j].y > point.y)) &&
			(point.x < (vertices[j].x - vertices[i].x) * (point.y - vertices[i].y) / (vertices[j].y - vertices[i].y) + vertices[i].x)) {
			inside = !inside;
		}
	}
	return inside;
}

inline sf::Vector2f RandomPointInPolygon(const std::vector<sf::Vector2f>& vertices) {
	// DO NOT USE!! This function is slow. It uses rejection sampling to get a random point in a polygon
	// Get a random point within a polygon
	float xMin = vertices[0].x;
	float xMax = vertices[0].x;
	float yMin = vertices[0].y;
	float yMax = vertices[0].y;

    for (int i = 1; i < vertices.size(); i++) {
        if (vertices[i].x < xMin) {
			xMin = vertices[i].x;
		}
        if (vertices[i].x > xMax) {
			xMax = vertices[i].x;
		}
        if (vertices[i].y < yMin) {
			yMin = vertices[i].y;
		}
        if (vertices[i].y > yMax) {
			yMax = vertices[i].y;
		}
	}

	sf::Vector2f point;
    do {
		point.x = RandomBetween(xMin, xMax);
		point.y = RandomBetween(yMin, yMax);
	} while (!pointInPolygon(point, vertices));

	return point;
}

std::vector<sf::Vector2f> convexHull(const std::vector<sf::Vector2f>& points);

// Function to compute the centroid of a polygon
sf::Vector2f computeCentroid(const std::vector<sf::Vector2f>& points);

// Function to compute the angle between two points and the centroid
inline float computeAngle(const sf::Vector2f& centroid, const sf::Vector2f& point) 
{
    return std::atan2(point.y - centroid.y, point.x - centroid.x);
}

// Function to order points sequentially around the centroid
std::vector<sf::Vector2f> orderPoints(std::vector<sf::Vector2f> points);


// Function to compute the area of a polygon (with unordered points)
float polygonArea(const std::vector<sf::Vector2f>& points);


// Color table with names from https://www.rapidtables.com/web/color/RGB_Color.html
class ColorTable {
private:
    std::map<std::string, sf::Color> colorMap;
public:
    sf::Color getColor(std::string name) {
		return colorMap[name];
	}
    std::string getName(sf::Color color) {
        for (auto const& pair : colorMap) {
            if (pair.second == color) {
				return pair.first;
			}
		}
		return "unknown";
	}
    std::string getClosestName(sf::Color color)
    {
        float minDist = std::numeric_limits<float>::max();
        std::string minIndex = "Unknown";
        for (auto const& pair : colorMap) {
			float dist = sqrt(pow(color.r - pair.second.r, 2) + pow(color.g - pair.second.g, 2) + pow(color.b - pair.second.b, 2));
            if (dist < minDist) {
				minDist = dist;
				minIndex = pair.first;
			}
		}
        return minIndex;
    }
    sf::Color getRandomColor() {
		int index = rand() % colorMap.size();
		auto it = colorMap.begin();
		std::advance(it, index);
		return it->second;
	}
    std::vector<sf::Color> getRandomColors(int numColors) {
        // Without replacement
        if (numColors > colorMap.size()) {
            throw std::invalid_argument("Number of colors requested exceeds number of colors in the map");
        }
        std::vector<sf::Color> colors;
        int size = colorMap.size();
        std::vector<int> usedIndex = {};
        for (auto const& pair : colorMap) {
            int index = rand() % size;
            while (std::find(usedIndex.begin(), usedIndex.end(), index) != usedIndex.end()) {
				index = rand() % size;
			}
            auto it = colorMap.begin();
            std::advance(it, index);
            colors.push_back(it->second);
            usedIndex.push_back(index);
        }
		return colors;
    }
    ColorTable() {
		colorMap["red"] = sf::Color(255, 0, 0);
		colorMap["green"] = sf::Color(0, 128, 0);
		colorMap["blue"] = sf::Color(0, 0, 255);
		colorMap["yellow"] = sf::Color(255, 255, 0);
		colorMap["orange"] = sf::Color(255, 165, 0);
		colorMap["purple"] = sf::Color(128, 0, 128);
		colorMap["cyan"] = sf::Color(0, 255, 255);
		colorMap["magenta"] = sf::Color(255, 0, 255);
		colorMap["lime"] = sf::Color(0, 255, 0);
		colorMap["pink"] = sf::Color(255, 192, 203);
		colorMap["teal"] = sf::Color(0, 128, 128);
		colorMap["lavender"] = sf::Color(230, 230, 250);
		colorMap["brown"] = sf::Color(165, 42, 42);
		colorMap["beige"] = sf::Color(245, 245, 220);
		colorMap["maroon"] = sf::Color(128, 0, 0);
		colorMap["mint"] = sf::Color(189, 252, 201);
		colorMap["apricot"] = sf::Color(251, 206, 177);
		colorMap["navy"] = sf::Color(0, 0, 128);
		colorMap["grey"] = sf::Color(128, 128, 128);
		colorMap["white"] = sf::Color(255, 255, 255);
		colorMap["black"] = sf::Color(0, 0, 0);
        colorMap["gold"] = sf::Color(255, 215, 0);
        colorMap["silver"] = sf::Color(192, 192, 192);
        colorMap["bronze"] = sf::Color(205, 127, 50);
        colorMap["lightblue"] = sf::Color(173, 216, 230);
        colorMap["lightgreen"] = sf::Color(144, 238, 144);
        colorMap["lightyellow"] = sf::Color(255, 255, 224);
        colorMap["lightorange"] = sf::Color(255, 160, 122);
        colorMap["lightpurple"] = sf::Color(221, 160, 221);
        colorMap["lightcyan"] = sf::Color(224, 255, 255);
        colorMap["lightmagenta"] = sf::Color(255, 224, 255);
        colorMap["lightlime"] = sf::Color(144, 238, 144);
        colorMap["lightpink"] = sf::Color(255, 182, 193);
        colorMap["lightteal"] = sf::Color(173, 216, 230);
        colorMap["lightlavender"] = sf::Color(230, 230, 250);
        colorMap["lightbrown"] = sf::Color(165, 42, 42);
        colorMap["lightbeige"] = sf::Color(245, 245, 220);
        colorMap["lightmaroon"] = sf::Color(128, 0, 0);
        colorMap["lightmint"] = sf::Color(189, 252, 201);
        colorMap["lightapricot"] = sf::Color(251, 206, 177);
        colorMap["lightnavy"] = sf::Color(0, 0, 128);
        colorMap["lightgrey"] = sf::Color(128, 128, 128); 
	}
};

sf::Color randomColor();

std::vector<sf::Color> randomColors(int numColors);

std::string colorName(sf::Color color);

std::string closestColorName(sf::Color color);

sf::Color colorByName(std::string name);

sf::Color closeRandomColorChange(sf::Color color);

// Based on Stefan Gustavson's implementation
class SimplexNoise {
private:
    int perm[512];
    int grad3[12][3] = {
        {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
        {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
        {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
    };

    float dot(const int g[3], float x, float y, float z) {
        return g[0] * x + g[1] * y + g[2] * z;
    }

    int fastfloor(float x) {
        return x > 0 ? (int)x : (int)x - 1;
    }

public:
    SimplexNoise(int seed) {
        // Initialize permutation table with values based on seed
        std::mt19937 gen(seed);
        std::uniform_int_distribution<int> distrib(0, 255);

        for (int i = 0; i < 256; i++) {
            perm[i] = distrib(gen);
            perm[i + 256] = perm[i];
        }
    }

    float noise(float x, float y) {
        // 2D simplex noise
        const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
        const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;

        // Skew input space to determine which simplex cell we're in
        float s = (x + y) * F2;
        int i = fastfloor(x + s);
        int j = fastfloor(y + s);

        float t = (i + j) * G2;
        float X0 = i - t; // Unskew the cell origin back to (x,y) space
        float Y0 = j - t;
        float x0 = x - X0; // The x,y distances from the cell origin
        float y0 = y - Y0;

        // Determine which simplex we are in
        int i1, j1; // Offsets for second corner of simplex
        if (x0 > y0) { // lower triangle, XY order: (0,0)->(1,0)->(1,1)
            i1 = 1;
            j1 = 0;
        }
        else { // upper triangle, YX order: (0,0)->(0,1)->(1,1)
            i1 = 0;
            j1 = 1;
        }

        // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
        // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
        // c = (3-sqrt(3))/6

        float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2; // Offsets for last corner in (x,y) unskewed coords
        float y2 = y0 - 1.0f + 2.0f * G2;

        // Work out the hashed gradient indices of the three simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int gi0 = perm[ii + perm[jj]] % 12;
        int gi1 = perm[ii + i1 + perm[jj + j1]] % 12;
        int gi2 = perm[ii + 1 + perm[jj + 1]] % 12;

        // Calculate the contribution from the three corners
        float n0, n1, n2;

        // Calculate noise contributions from each corner
        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 < 0) {
            n0 = 0.0f;
        }
        else {
            t0 *= t0;
            n0 = t0 * t0 * dot(grad3[gi0], x0, y0, 0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 < 0) {
            n1 = 0.0f;
        }
        else {
            t1 *= t1;
            n1 = t1 * t1 * dot(grad3[gi1], x1, y1, 0);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 < 0) {
            n2 = 0.0f;
        }
        else {
            t2 *= t2;
            n2 = t2 * t2 * dot(grad3[gi2], x2, y2, 0);
        }

        // Add contributions from each corner to get the final noise value.
        // The result is scaled to return values in the range [-1,1]
        return 70.0f * (n0 + n1 + n2);
    }

    // Generate octave noise (multiple frequencies of noise added together)
    float octaveNoise(float x, float y, int octaves, float persistence) {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f;  // Used for normalizing result

        for (int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }
};


inline float distance(sf::Vector2f a, sf::Vector2f b)
{
    // Euclidian distance
	return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}


static float magnitude(const sf::Vector2f& vec) {
    return std::sqrt(vec.x * vec.x + vec.y * vec.y);
}
static sf::Vector2f normalize(const sf::Vector2f& vec) {
    float mag = magnitude(vec);
    if (mag < 0.001f) return sf::Vector2f(0.0f, 0.0f);
    return sf::Vector2f(vec.x / mag, vec.y / mag);
}





#endif // UTIL_HPP


