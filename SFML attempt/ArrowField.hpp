#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <SFML/Graphics/Color.hpp>
#include "SphereVoronoi.hpp"

class Continent;

// Arrows over the planet, the wind and the plate motion, spread evenly at any zoom.
//
// Level 0 has an arrow on every cell. Each coarser level is a Fibonacci lattice with a quarter
// of the points, so twice the spacing; every lattice point snaps to its nearest cell, the anchor,
// and every cell belongs to the region of the anchor it is closest to over the cell graph. The
// view picks the finest level whose arrows are at least TARGET_SPACING_PX apart on screen.
//
// Arrows are sized on screen as a fraction of the slot, the pixel spacing of their level, so
// zooming only changes a uniform and the vertices are rebuilt when the level changes.
struct ArrowLevel {
    std::vector<int> anchors; // Cell of each arrow
    std::vector<int> region;  // Arrow index of every cell
    float spacing = 0.f;      // Radians, sqrt(4 pi / arrows)
};

constexpr float ARROW_TARGET_SPACING_PX = 40.f;
constexpr int ARROW_MIN_COUNT = 12; // Coarsest level

std::vector<ArrowLevel> buildArrowLevels(const vor::SphereVoronoi& planet);

// Finest level whose spacing is at least targetPx at zoom pixels per radian, else the coarsest
int arrowLevelFor(const std::vector<ArrowLevel>& levels, float zoom, float targetPx = ARROW_TARGET_SPACING_PX);
// Slot in pixels for sizing the arrows of a level: its spacing on screen, capped at twice the
// target so the finest level doesn't grow without end when zoomed far in
float arrowSlotPx(const ArrowLevel& level, float zoom, float targetPx = ARROW_TARGET_SPACING_PX);

struct Arrow {
    glm::vec3 anchor;    // Unit sphere
    glm::vec3 direction; // Unit tangent at the anchor
    float size;          // Length as a fraction of the slot
    sf::Color color;
};

// Mean wind of each region: the mean of the cells' unit wind tangents, put in the anchor's
// tangent plane, with the mean strength. Coloured blue to red by strength like the flat map
std::vector<Arrow> windArrows(const vor::SphereVoronoi& planet, const ArrowLevel& level);
// Plate motion at each anchor (not averaged, so a rotating plate shows its swirl), black, the
// length by speed relative to maxSpeed
std::vector<Arrow> plateArrows(const vor::SphereVoronoi& planet, const std::vector<Continent>& plates, const ArrowLevel& level, float maxSpeed);
// Fastest surface speed of any cell's plate, for scaling the plate arrows
float maxPlateSpeed(const vor::SphereVoronoi& planet, const std::vector<Continent>& plates);

sf::Color windColor(float strength);

// Three vertices per arrow, every one with the anchor and a point a little ahead of it
struct ArrowVertex {
    glm::vec3 position;
    glm::vec2 latLon;      // Radians
    glm::vec3 aheadPosition;
    glm::vec2 aheadLatLon; // Radians, lon unwrapped next to the anchor's
    sf::Color color;
    float size;
    float corner;          // 0 tip, 1 and 2 base
};

struct ArrowBatch {
    std::vector<ArrowVertex> vertices;
    float lonMin = 0.f, lonMax = 0.f;
    void add(const std::vector<Arrow>& arrows);
};
