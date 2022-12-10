#pragma once
#include <vector>

int long rand_long()
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

    int rand_index = rand() % v.size();
    int value = v[rand_index];
    v.erase(std::next(v.begin(), rand_index));
    return value;
}



