#pragma once

#include <SFML/System/Vector2.hpp>
#include <vector>
#include <math.h>

// Initialize vector of points that act as the "centers/ids" for each cell
std::vector<sf::Vector2f> points;

inline double dist(sf::Vector2f a, sf::Vector2f b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

float clamp(float x, float max, float min) {
    if (x < min) { return min; }
    if (x > max) { return max; }
    return x;
}

// Perlin noise, will be used for height?? or something
float interpolate(float a0, float a1, float w) {
    w = clamp(w, 1.f, 0.f); // Just to make sure
    return (a1 - a0) * w + a0;
    // Using the smoothstep function:
    // return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
    // Smoother step could also have been used
    return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
}

sf::Vector2f randomGradient() {
    sf::Vector2f v;
    // Not completely random because it is two random combined, not just random between -1 and 1
    int randomSignx = rand() % 2;
    int randomSigny = rand() % 2;
    float randomx = (float)rand() / (float)RAND_MAX;
    float randomy = (float)rand() / (float)RAND_MAX;
    v.x = (randomSignx) * randomx + (randomSignx-1)*randomx;
    v.y = (randomSigny)*randomy + (randomSigny - 1) * randomy;
    return v;
}

float dotGridGradient(int ix, int iy, float x, float y) {
    // ix and iy are the grid coordinates
    // get gradient
    sf::Vector2f gradient = randomGradient();

    // Compute the distance
    float dx = x - (float)ix;
    float dy = y - (float)iy;

    return (dx * gradient.x + dy * gradient.y);
}

// Perlin noise is not the way I think... I dont like the look of it
float perlin(float x, float y) {
    // Grid cell coordinates
    int x0 = (int)floor(x);
    int x1 = x0 + 1;
    int y0 = (int)floor(y);
    int y1 = y0 + 1;

    // Determine interpolation weights
    float sx = x - (float)x0;
    float sy = y - (float)y0;

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix0 = interpolate(n0, n1, sx);
    
    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);
    return (value+1); // returns value between -1 and 1
}

void generatePoints(std::vector<sf::Vector2f>& points,int ncellsx,int ncellsy,int MAXWIDTH,int MAXHEIGHT,float jitter) {

    float stepSizewidth = (float)MAXWIDTH / ncellsx;
    float stepSizeHeight = (float)MAXHEIGHT / ncellsy;

    for (int x = 0; x < MAXWIDTH; x = x + stepSizewidth) {
        for (int y = 0; y < MAXHEIGHT; y = y + stepSizeHeight) {
            sf::Vector2f p;
            sf::Vector2f random = randomGradient()*jitter;

            p.x = clamp(x + random.x, MAXWIDTH, 0);
            p.y = clamp(y + random.y, MAXHEIGHT, 0);
            points.push_back(p);

        }
    }
}


