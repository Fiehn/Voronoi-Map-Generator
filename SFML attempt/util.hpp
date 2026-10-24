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
    // Box-Muller transform. u1 must not be 0, log(0) gave infinite values once in 32768 draws
	float u1 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 1.0f);
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
        // Without replacement until the table runs out, after that the shuffled table repeats
        std::vector<sf::Color> table;
        for (auto const& pair : colorMap) {
            table.push_back(pair.second);
        }
        for (int i = static_cast<int>(table.size()) - 1; i > 0; i--) {
            std::swap(table[i], table[rand() % (i + 1)]);
        }
        std::vector<sf::Color> colors;
        for (int i = 0; i < numColors; i++) {
            colors.push_back(table[i % table.size()]);
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

    float noise(float x, float y, float z) {
        // 3D simplex noise, for sampling on the sphere at p * frequency
        const float F3 = 1.0f / 3.0f;
        const float G3 = 1.0f / 6.0f;

        // Skew input space to determine which simplex cell we're in
        float s = (x + y + z) * F3;
        int i = fastfloor(x + s);
        int j = fastfloor(y + s);
        int k = fastfloor(z + s);

        float t = (i + j + k) * G3;
        float x0 = x - (i - t); // The x,y,z distances from the cell origin
        float y0 = y - (j - t);
        float z0 = z - (k - t);

        // Determine which of the six simplices we are in from the order of x0, y0, z0
        int i1, j1, k1; // Offsets for second corner of simplex
        int i2, j2, k2; // Offsets for third corner of simplex
        if (x0 >= y0) {
            if (y0 >= z0)      { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // X Y Z order
            else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; } // X Z Y order
            else               { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; } // Z X Y order
        }
        else {
            if (y0 < z0)       { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; } // Z Y X order
            else if (x0 < z0)  { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; } // Y Z X order
            else               { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // Y X Z order
        }

        float x1 = x0 - i1 + G3; // Offsets for second corner
        float y1 = y0 - j1 + G3;
        float z1 = z0 - k1 + G3;
        float x2 = x0 - i2 + 2.0f * G3; // Offsets for third corner
        float y2 = y0 - j2 + 2.0f * G3;
        float z2 = z0 - k2 + 2.0f * G3;
        float x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner
        float y3 = y0 - 1.0f + 3.0f * G3;
        float z3 = z0 - 1.0f + 3.0f * G3;

        // Work out the hashed gradient indices of the four simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
        int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
        int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
        int gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

        // Calculate the contribution from the four corners
        float n0, n1, n2, n3;

        float t0 = 0.5f - x0 * x0 - y0 * y0 - z0 * z0;
        if (t0 < 0) {
            n0 = 0.0f;
        }
        else {
            t0 *= t0;
            n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1 - z1 * z1;
        if (t1 < 0) {
            n1 = 0.0f;
        }
        else {
            t1 *= t1;
            n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2 - z2 * z2;
        if (t2 < 0) {
            n2 = 0.0f;
        }
        else {
            t2 *= t2;
            n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
        }

        float t3 = 0.5f - x3 * x3 - y3 * y3 - z3 * z3;
        if (t3 < 0) {
            n3 = 0.0f;
        }
        else {
            t3 *= t3;
            n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
        }

        // Add contributions from each corner to get the final noise value.
        // The kernel radius^2 is 0.5 rather than the 0.6 in Gustavson's paper, 0.6 leaves small seams.
        // 76 is the measured peak over 16M samples, which keeps the result just inside [-1,1]
        return 76.0f * (n0 + n1 + n2 + n3);
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

    float octaveNoise(float x, float y, float z, int octaves, float persistence) {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f;  // Used for normalizing result

        for (int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }
};






#endif // UTIL_HPP


