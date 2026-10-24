#include "Ribbon.hpp"
#include "MapProjection.hpp"
#include <cmath>
#include <algorithm>

namespace {
    constexpr float DUPLICATE_DISTANCE = 1e-7f; // Chord length, far below a pixel at any zoom

    // (lat, lon) in degrees, in double like the planet mesh so shared points agree
    glm::dvec2 latLonOf(const glm::vec3& p)
    {
        const double x = p.x, y = p.y, z = p.z;
        return glm::dvec2(glm::degrees(std::atan2(y, std::sqrt(x * x + z * z))), glm::degrees(std::atan2(x, z)));
    }
}

void RibbonBatch::clear()
{
    vertices.clear();
    indices.clear();
    discs.clear();
    lonMin = lonMax = 0.f;
}

void RibbonBatch::extendLonRange(float lon)
{
    if (vertices.empty() && discs.empty()) {
        lonMin = lonMax = lon;
        return;
    }
    lonMin = std::min(lonMin, lon);
    lonMax = std::max(lonMax, lon);
}

void RibbonBatch::addLine(const std::vector<glm::vec3>& input, bool closed, sf::Color color, float widthPx)
{
    addLineImpl(input, std::vector<float>(input.size(), widthPx), closed, color);
}

void RibbonBatch::addLine(const std::vector<glm::vec3>& input, const std::vector<float>& widthsPx, bool closed, sf::Color color)
{
    addLineImpl(input, widthsPx, closed, color);
}

void RibbonBatch::addLineImpl(const std::vector<glm::vec3>& input, const std::vector<float>& widthsPx, bool closed, sf::Color color)
{
    std::vector<glm::vec3> p;
    std::vector<float> w;
    p.reserve(input.size());
    for (std::size_t k = 0; k < input.size(); k++) {
        if (p.empty() || glm::length(input[k] - p.back()) > DUPLICATE_DISTANCE) {
            p.push_back(input[k]);
            w.push_back(k < widthsPx.size() ? widthsPx[k] : 1.f);
        }
    }
    if (closed && p.size() > 1 && glm::length(p.front() - p.back()) <= DUPLICATE_DISTANCE) { p.pop_back(); w.pop_back(); }

    const int n = static_cast<int>(p.size());
    if (n < 2) return;
    if (closed && n < 3) closed = false;

    // Each step's longitude unwrapped from the step before
    std::vector<glm::dvec2> ll(n);
    for (int i = 0; i < n; i++) {
        ll[i] = latLonOf(p[i]);
        if (i > 0) ll[i].y = mapproj::unwrapLon(ll[i].y, ll[i - 1].y);
    }
    // Whole turns a closed line goes round, by the step from its last point back to its first
    const double turn = closed ? mapproj::unwrapLon(ll[0].y, ll[n - 1].y) - ll[0].y : 0.0;

    const unsigned int first = static_cast<unsigned int>(vertices.size());
    const auto emit = [&](int i, double lonShift) {
        const glm::vec2 latLon(static_cast<float>(glm::radians(ll[i].x)), static_cast<float>(glm::radians(ll[i].y + lonShift)));
        extendLonRange(latLon.y);
        vertices.push_back({ p[i], latLon, color, 0.5f * w[i], -1.f });
        vertices.push_back({ p[i], latLon, color, 0.5f * w[i], 1.f });
    };

    if (closed) emit(n - 1, -turn);
    else emit(0, 0.0);
    for (int i = 0; i < n; i++) emit(i, 0.0);
    if (closed) {
        emit(0, turn);
        emit(1, turn);
    }
    else emit(n - 1, 0.0);

    // Segment s joins points s and s + 1, which sit at pairs s + 1 and s + 2. Their previous
    // slots are two vertices earlier, so the quad's indices start at 2s
    const int segments = closed ? n : n - 1;
    for (int s = 0; s < segments; s++) {
        const unsigned int v = first + 2 * static_cast<unsigned int>(s);
        indices.insert(indices.end(), { v, v + 1, v + 2, v + 1, v + 3, v + 2 });
    }
}

void RibbonBatch::addDisc(const glm::vec3& point, sf::Color color, float widthPx)
{
    const glm::dvec2 ll = latLonOf(point);
    const glm::vec2 latLon(static_cast<float>(glm::radians(ll.x)), static_cast<float>(glm::radians(ll.y)));
    extendLonRange(latLon.y);
    discs.push_back({ point, latLon, color, 0.5f * widthPx, 0.f });
}
