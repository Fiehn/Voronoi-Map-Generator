#pragma once
#include <vector>
#include <string>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <map>

inline long rand_long() // Should only be used in the case that there is a need for larger variables
{
    return rand() << 15 | rand();
}

// long RAND_LONG_MAX = RAND_MAX << 15 | RAND_MAX;

inline float clamp(float x, float max, float min) 
{
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
    void push(const T& value) { m_queue.push_back(value); }
    T pop_front() {
        if (m_queue.empty()) 
        { 
            return 0; // This is not type safe
        }
        T value = m_queue[front_index];
        ++front_index;

        if (front_index > m_queue.size() / 2) {
			m_queue.erase(m_queue.begin(), m_queue.begin() + front_index);
			front_index = 0;
		}
        return value;
    }
    bool empty() const { return (front_index == m_queue.size() or m_queue.size() == 0); }
    size_t size() const { return m_queue.size() - front_index; }

    T pop_random() {
		if (m_queue.empty()) { return 0; }
		size_t rand_index = front_index + (rand_long() % (m_queue.size() - front_index));
		std::swap(m_queue[rand_index], m_queue.back());
		T value = m_queue.back();
		m_queue.pop_back();
		return value;
	}

    //void clear() { m_queue.clear(); front_index = 0; }
    //T& operator[](size_t index) { return m_queue[index + front_index]; }
    //const T& operator[](size_t index) const { return m_queue[index + front_index]; }
    //T& front() { return m_queue[front_index]; }
    //const T& front() const { return m_queue[front_index]; }
    //T& back() { return m_queue[m_queue.size() - 1]; }
    //const T& back() const { return m_queue[m_queue.size() - 1]; }
    //void erase(size_t index) { m_queue.erase(m_queue.begin() + index + front_index); }
    //void erase(size_t index, size_t count) { m_queue.erase(m_queue.begin() + index + front_index, m_queue.begin() + index + front_index + count); }
private:
    std::vector<T> m_queue;
    std::size_t front_index = 0;
};

inline float normalized_value(float value, float max, float min) { return fabs((value - min) / (max - min)); }

// Is this the correct way?
template <typename T>
void insert_unique(std::vector<T>& vec, const T& key) {
    if (std::find(vec.begin(), vec.end(), key) == vec.end()) {
        vec.push_back(key);
    }
}

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

    return sf::Vector2f(x, y);
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

float normalizeAngle(float angleDegrees) {
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
	return degrees * (PI / 180.0);
}

std::vector<float> scalarMultiplication(const std::vector<float>& vec, float scalar) {
    std::vector<float> result;
    result.reserve(vec.size()); // Reserve space for efficiency

    // Perform scalar multiplication
    for (float value : vec) {
        result.push_back(value * scalar);
    }

    return result;
}

inline float sumf(const std::vector<float>& vec) {
	float sum = 0;
    for (float value : vec) {
		sum += value;
	}
	return sum;
}

int chooseIndexMax(std::vector<float> rates) {
    // get index of max value of rates
    return std::max_element(rates.begin(), rates.end()) - rates.begin();
}

int chooseIndex(std::vector<float> rates) {
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

float normalDistPDF(float mean, float std)
{
    // Box-Muller transform
	float u1 = (float)rand() / RAND_MAX;
	float u2 = (float)rand() / RAND_MAX;
	return mean + std * sqrt(-2 * log(u1)) * cos(2 * PI * u2);
}

float betaDist(float alpha, float beta)
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






