#pragma once


# define M_PI           3.14159265358979323846  /* pi */

//
// Poisson Disk Sampling Bridson's Algorithm
void insertPoint(std::vector<std::vector<sf::Vector2f>>& grid, float cellsize, sf::Vector2f point) {
    int xindex = floor(point.x / cellsize);
    int yindex = floor(point.y / cellsize);
    grid[xindex][yindex] = point;
}
bool isValidPoint(std::vector<std::vector<sf::Vector2f>>& grid, float cellsize, int width, int height, sf::Vector2f p, float radius) {
    // make sure the point is within box
    if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) { return false; }

    // check neighborhood cells
    int xindex = floor(p.x / cellsize);
    int yindex = floor(p.y / cellsize);
    int i0 = std::max(xindex - 1, 0);
    int i1 = std::min(xindex + 1, width - 1);
    int j0 = std::max(yindex - 1, 0);
    int j1 = std::min(yindex + 1, height - 1);

    for (int i = i0; i <= i1; i++) {
        for (int j = j0; j <= j1; j++) {
            if (grid[i][j] != sf::Vector2f(-1.f, -1.f)) {
                if (dist(grid[i][j], p) < radius) {
                    return false;
                }
            }
        }
    }
    return true;
}
void poissonDiskSampling(std::vector<sf::Vector2f>& points, float radius, int k, int width, int height) {
    // radius is minimum distance and k is the limit
    int N = 2; // 2d
    std::vector<sf::Vector2f> active;
    sf::Vector2f p0;

    // Grid initialization
    float cellsize = floor(radius / sqrt(N));
    // number of cells in the grid for given canvas
    int ncells_width = ceil(width / cellsize) + 1;
    int ncells_height = ceil(height / cellsize) + 1;
    // Initialize grid with 2d vectors storing points in each index
    std::vector<std::vector<sf::Vector2f>> grid(ncells_width, std::vector<sf::Vector2f>(ncells_height, sf::Vector2f(-1.f, -1.f))); // default correct??

    // Insert first point
    insertPoint(grid, cellsize, p0);
    points.push_back(p0);
    active.push_back(p0);

    while (active.empty() == false) {
        int random_index = rand() % active.size();
        sf::Vector2f p = active[random_index];

        bool found = false;
        for (int tries = 0; tries < k; tries++) {
            // If valid point found
            float theta = ((float)rand() / (float)RAND_MAX) * 2 * M_PI;
            float new_radius = ((float)rand() / (float)RAND_MAX) * radius + radius;
            float pnewx = p.x + new_radius * cos(theta);
            float pnewy = p.y + new_radius * sin(theta);
            sf::Vector2f pnew(pnewx, pnewy);

            if (!isValidPoint(grid, cellsize, width, height, pnew, radius)) { continue; }

            points.push_back(pnew);
            insertPoint(grid, cellsize, pnew);
            active.push_back(pnew);
            found = true;
            break;
        }

        // maybe bug?
        if (!found) { active.erase(std::next(active.begin(), random_index)); }

    }

    //return points;
}





// Perlin noise is also here
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
    v.x = (randomSignx)*randomx + (randomSignx - 1) * randomx;
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
    return (value + 1); // returns value between -1 and 1
}
