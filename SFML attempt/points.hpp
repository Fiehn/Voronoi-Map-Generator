#pragma once

#include <SFML/System/Vector2.hpp>
#include <vector>
#include <math.h>
#include "util.h"


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


