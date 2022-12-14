#pragma once
#include <vector>

long rand_long() // Should only be used in the case that there is a need for larger variables
{
    return rand() << 15 | rand();
}

long RAND_LONG_MAX = RAND_MAX << 15 | RAND_MAX;

float clamp(float x, float max, float min) 
{
    if (x < min) { return min; }
    if (x > max) { return max; }
    return x;
}
sf::Vector2f clampVect2f(sf::Vector2f vect, float maxx, float minx, float maxy, float miny)
{
    sf::Vector2f new_vect = vect;
    if (vect.x < minx) { new_vect.x = minx; }
    if (vect.x > maxx) { new_vect.x = maxy; }
    if (vect.y < miny) { new_vect.y = miny; }
    if (vect.y > maxy) { new_vect.y = maxy; }
    return new_vect;
}


float RandomBetween(float smallNumber, float bigNumber)
{
    float diff = bigNumber - smallNumber;
    return (((float)rand() / RAND_MAX) * diff) + smallNumber;
}

int pop_front_i(std::vector<int>& v)
{
    if (v.empty()) { return 0; }

    int value = v[0];
    v.erase(v.begin());
    return value;

}

int pop_random_i(std::vector<int>& v)
{
    if (v.empty()) { return 0; }

    int rand_index = rand_long() % v.size();
    int value = v[rand_index];
    v.erase(std::next(v.begin(), rand_index));
    return value;
}



