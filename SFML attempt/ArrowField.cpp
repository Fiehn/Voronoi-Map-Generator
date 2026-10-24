#include "ArrowField.hpp"
#include "CellObjects.hpp"
#include "SphereMath.hpp"
#include "MapProjection.hpp"
#include <cmath>
#include <algorithm>

namespace {
    constexpr double PI_D = 3.14159265358979323846;
    constexpr float ARROW_SIZE = 0.8f;    // Full strength arrows are this fraction of the slot
    constexpr float AHEAD_STEP = 1e-3f;   // Radians to the point ahead, enough for float, far below a pixel
    constexpr float MIN_DIRECTION = 1e-4f; // Mean directions shorter than this cancel out, no arrow

    // Points of the golden spiral, the same lattice fibonacciPoints builds but without jitter
    // or rand(), so drawing arrows doesn't change the map's random sequence
    std::vector<glm::vec3> spiral(int n)
    {
        std::vector<glm::vec3> points(n);
        const double golden = PI_D * (3.0 - std::sqrt(5.0));
        for (int i = 0; i < n; i++) {
            const double y = 1.0 - 2.0 * (i + 0.5) / n;
            const double r = std::sqrt(std::max(0.0, 1.0 - y * y));
            const double theta = golden * i;
            points[i] = glm::vec3(static_cast<float>(r * std::sin(theta)), static_cast<float>(y), static_cast<float>(r * std::cos(theta)));
        }
        return points;
    }

    // Starting cells for findCell on a coarse lat/lon grid. Consecutive spiral points lie the
    // golden angle apart, so walking from the previous point's cell crosses half the planet
    struct GuessGrid {
        static constexpr int ROWS = 64, COLUMNS = 128;
        std::vector<int> cells = std::vector<int>(ROWS * COLUMNS, -1);

        static int binOf(const glm::vec3& p)
        {
            const double lat = std::asin(std::clamp(static_cast<double>(p.y), -1.0, 1.0));
            const double lon = std::atan2(static_cast<double>(p.x), static_cast<double>(p.z));
            const int row = std::clamp(static_cast<int>((lat / PI_D + 0.5) * ROWS), 0, ROWS - 1);
            const int column = std::clamp(static_cast<int>((lon / (2.0 * PI_D) + 0.5) * COLUMNS), 0, COLUMNS - 1);
            return row * COLUMNS + column;
        }
        explicit GuessGrid(const vor::SphereVoronoi& planet)
        {
            for (int i = 0; i < static_cast<int>(planet.points.size()); i++) cells[binOf(planet.points[i])] = i;
        }
        int guess(const glm::vec3& p, int fallback) const
        {
            const int cell = cells[binOf(p)];
            return cell >= 0 ? cell : fallback;
        }
    };

    glm::vec2 latLonRadians(const glm::vec3& p, double referenceLon)
    {
        const double x = p.x, y = p.y, z = p.z;
        const double lat = glm::degrees(std::atan2(y, std::sqrt(x * x + z * z)));
        const double lon = mapproj::unwrapLon(glm::degrees(std::atan2(x, z)), referenceLon);
        return glm::vec2(static_cast<float>(glm::radians(lat)), static_cast<float>(glm::radians(lon)));
    }
}

std::vector<ArrowLevel> buildArrowLevels(const vor::SphereVoronoi& planet)
{
    std::vector<ArrowLevel> levels;
    const int n = static_cast<int>(planet.cells.size());
    if (n == 0) return levels;

    ArrowLevel finest;
    finest.anchors.resize(n);
    finest.region.resize(n);
    for (int i = 0; i < n; i++) finest.anchors[i] = finest.region[i] = i;
    finest.spacing = static_cast<float>(std::sqrt(4.0 * PI_D / n));
    levels.push_back(std::move(finest));

    const GuessGrid guesses(planet);
    std::vector<int> queue;
    for (int count = n / 4; count >= ARROW_MIN_COUNT; count /= 4) {
        ArrowLevel level;
        level.region.assign(n, -1);
        // Snap the lattice to cells; two points in one cell (rare) make one arrow
        int previous = 0;
        for (const glm::vec3& p : spiral(count)) {
            const int cell = planet.findCell(p, guesses.guess(p, previous));
            previous = cell;
            if (cell < 0 || level.region[cell] >= 0) continue;
            level.region[cell] = static_cast<int>(level.anchors.size());
            level.anchors.push_back(cell);
        }
        // Regions grow from all anchors at once, a cell joins the first region to reach it
        queue.assign(level.anchors.begin(), level.anchors.end());
        for (std::size_t head = 0; head < queue.size(); head++) {
            const int cell = queue[head];
            for (int nb : planet.cells[cell].neighbors) {
                if (level.region[nb] >= 0) continue;
                level.region[nb] = level.region[cell];
                queue.push_back(nb);
            }
        }
        level.spacing = static_cast<float>(std::sqrt(4.0 * PI_D / level.anchors.size()));
        levels.push_back(std::move(level));
    }
    return levels;
}

int arrowLevelFor(const std::vector<ArrowLevel>& levels, float zoom, float targetPx)
{
    for (int k = 0; k < static_cast<int>(levels.size()); k++) {
        if (zoom * levels[k].spacing >= targetPx) return k;
    }
    return static_cast<int>(levels.size()) - 1;
}

float arrowSlotPx(const ArrowLevel& level, float zoom, float targetPx)
{
    return std::min(zoom * level.spacing, 2.f * targetPx);
}

sf::Color windColor(float strength)
{
    // Weak wind: blue -> cyan -> yellow -> red: strong wind
    const float s = std::clamp(strength, 0.f, 1.f);
    if (s < 0.33f) return sf::Color(0, static_cast<sf::Uint8>(255 * (s / 0.33f)), 255, 200);
    if (s < 0.66f) {
        const float t = (s - 0.33f) / 0.33f;
        return sf::Color(static_cast<sf::Uint8>(255 * t), 255, static_cast<sf::Uint8>(255 * (1.f - t)), 200);
    }
    const float t = std::min((s - 0.66f) / 0.34f, 1.f);
    return sf::Color(255, static_cast<sf::Uint8>(255 * (1.f - t)), 0, 200);
}

std::vector<Arrow> windArrows(const vor::SphereVoronoi& planet, const ArrowLevel& level)
{
    const std::size_t count = level.anchors.size();
    std::vector<glm::dvec3> direction(count, glm::dvec3(0.0));
    std::vector<double> strength(count, 0.0);
    std::vector<int> cells(count, 0);
    for (std::size_t i = 0; i < planet.cells.size(); i++) {
        const int a = level.region[i];
        if (a < 0) continue;
        direction[a] += glm::dvec3(bearingToTangent(planet.points[i], planet.cells[i].windDir));
        strength[a] += planet.cells[i].windStr;
        cells[a]++;
    }

    std::vector<Arrow> arrows;
    arrows.reserve(count);
    for (std::size_t a = 0; a < count; a++) {
        if (cells[a] == 0) continue;
        const glm::dvec3 anchor(planet.points[level.anchors[a]]);
        const glm::dvec3 tangent = direction[a] - glm::dot(direction[a], anchor) * anchor;
        if (glm::length(tangent) < MIN_DIRECTION * cells[a]) continue;
        const float mean = static_cast<float>(strength[a] / cells[a]);
        arrows.push_back({ planet.points[level.anchors[a]], glm::vec3(glm::normalize(tangent)), ARROW_SIZE * std::clamp(mean, 0.f, 1.f), windColor(mean) });
    }
    return arrows;
}

float maxPlateSpeed(const vor::SphereVoronoi& planet, const std::vector<Continent>& plates)
{
    float fastest = 0.f;
    for (std::size_t i = 0; i < planet.cells.size(); i++) {
        const int plate = planet.cells[i].continent;
        if (plate < 0 || plate >= static_cast<int>(plates.size())) continue;
        fastest = std::max(fastest, glm::length(plates[plate].velocityAt(planet.points[i])));
    }
    return fastest;
}

std::vector<Arrow> plateArrows(const vor::SphereVoronoi& planet, const std::vector<Continent>& plates, const ArrowLevel& level, float maxSpeed)
{
    std::vector<Arrow> arrows;
    if (maxSpeed <= 0.f) return arrows;
    arrows.reserve(level.anchors.size());
    for (int cell : level.anchors) {
        const int plate = planet.cells[cell].continent;
        if (plate < 0 || plate >= static_cast<int>(plates.size())) continue;
        const glm::vec3 velocity = plates[plate].velocityAt(planet.points[cell]);
        const float speed = glm::length(velocity);
        if (speed < MIN_DIRECTION * maxSpeed) continue;
        arrows.push_back({ planet.points[cell], velocity / speed, ARROW_SIZE * speed / maxSpeed, sf::Color::Black });
    }
    return arrows;
}

void ArrowBatch::add(const std::vector<Arrow>& arrows)
{
    for (const Arrow& arrow : arrows) {
        const glm::vec3 ahead = glm::normalize(arrow.anchor + AHEAD_STEP * arrow.direction);
        const double x = arrow.anchor.x, z = arrow.anchor.z;
        const double anchorLon = glm::degrees(std::atan2(x, z));
        const glm::vec2 latLon = latLonRadians(arrow.anchor, anchorLon);
        const glm::vec2 aheadLatLon = latLonRadians(ahead, anchorLon);
        if (vertices.empty()) lonMin = lonMax = latLon.y;
        lonMin = std::min(lonMin, latLon.y);
        lonMax = std::max(lonMax, latLon.y);
        for (int corner = 0; corner < 3; corner++) {
            vertices.push_back({ arrow.anchor, latLon, ahead, aheadLatLon, arrow.color, arrow.size, static_cast<float>(corner) });
        }
    }
}
