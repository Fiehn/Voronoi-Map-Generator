#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/Color.hpp>

// Lines on the planet with a constant width in screen pixels, for both the globe and the flat
// view. The CPU side only, GlobeRenderer draws them.
//
// Every point of a line is stored twice, once per side, and the vertex shader pushes each copy
// half the width out along the miter, worked out on screen from the points before and after.
// Those come from the same buffer bound three times: vertex v reads its previous point at v,
// itself at v + 2 and its next point at v + 4, so indices name the previous slot and every line
// is padded with one extra point at each end. Open lines pad with their end points (the shader
// then uses the one direction it has, a butt end), closed lines with their neighbours across
// the join.
//
// The longitude is unwrapped continuously along a line, so the flat view draws it without jumps.
// A closed line that goes round a pole ends a whole turn from where it began; its padding is
// shifted by that turn, so the flat view draws it once across the map and the globe still sees
// a closed loop.
struct RibbonVertex {
    glm::vec3 position;
    glm::vec2 latLon; // Radians, lon unwrapped along the line
    sf::Color color;
    float halfWidth;  // Pixels
    float side;       // -1 or 1
};

struct RibbonBatch {
    std::vector<RibbonVertex> vertices;
    std::vector<unsigned int> indices; // Triangles, each index names a vertex's previous slot
    std::vector<RibbonVertex> discs;   // Round points, the width is the diameter
    float lonMin = 0.f, lonMax = 0.f;  // Radians, of the vertices and discs drawn

    void clear();
    bool empty() const { return indices.empty() && discs.empty(); }

    // Points on the unit sphere. Repeated points are dropped, and so is the last point of a
    // closed line if it repeats the first
    void addLine(const std::vector<glm::vec3>& points, bool closed, sf::Color color, float widthPx);
    void addDisc(const glm::vec3& point, sf::Color color, float widthPx);

private:
    void extendLonRange(float lon);
};
