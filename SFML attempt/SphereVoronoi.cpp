#include "SphereVoronoi.hpp"
#include "Delaunator.hpp"
#include "SphereMath.hpp"
#include "util.hpp"
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <glm/gtc/constants.hpp>

namespace vor {

    std::vector<glm::vec3> fibonacciPoints(int n, float jitter)
    {
        std::vector<glm::vec3> points;
        points.reserve(n);

        const double goldenAngle = glm::pi<double>() * (3.0 - std::sqrt(5.0));
        // Side of a square with the area of one cell, so 1 matches the old grid step
        const float spacing = std::sqrt(4.0f * glm::pi<float>() / n);
        jitter = clamp(jitter, 1.0f, 0.0f);

        for (int i = 0; i < n; i++) {
            const double y = 1.0 - (2.0 * i + 1.0) / n; // north to south, never exactly a pole
            const double r = std::sqrt(1.0 - y * y);
            const double theta = goldenAngle * i;
            glm::vec3 p(static_cast<float>(r * std::sin(theta)), static_cast<float>(y), static_cast<float>(r * std::cos(theta)));

            if (jitter > 0.0f) {
                glm::vec3 east, north;
                tangentFrame(p, east, north);
                sf::Vector2f g = randomGradient() * (jitter * spacing);
                p = glm::normalize(p + g.x * east + g.y * north);
            }
            points.push_back(p);
        }
        return points;
    }

    void SphereVoronoi::fillMap(int n_cells, float point_jitter)
    {
        clearMap();
        points = fibonacciPoints(n_cells, point_jitter);
        triangulate();
        buildCells();
    }

    void SphereVoronoi::clearMap()
    {
        points.clear();
        cells.clear();
        voronoi_points.clear();
        cellLat.clear();
        cellLon.clear();
        triangles.clear();
        halfedges.clear();
    }

    void SphereVoronoi::triangulate()
    {
        const int n = static_cast<int>(points.size());
        if (n < 4) throw std::runtime_error("SphereVoronoi needs at least 4 points");

        // Stereographic projection from points[0]. Circles on the sphere stay circles, so the
        // planar Delaunay of the other points is the spherical one, minus the triangles around
        // points[0]. Those are the planar hull edges fanned to points[0].
        const glm::dvec3 pole = glm::normalize(glm::dvec3(points[0]));
        const glm::dvec3 helper = std::abs(pole.x) < 0.9 ? glm::dvec3(1, 0, 0) : glm::dvec3(0, 1, 0);
        const glm::dvec3 u = glm::normalize(glm::cross(helper, pole));
        const glm::dvec3 v = glm::cross(pole, u);

        std::vector<glm::dvec2> planar(n - 1);
        for (int i = 1; i < n; i++) {
            const glm::dvec3 q = glm::normalize(glm::dvec3(points[i]));
            const double d = 1.0 - glm::dot(q, pole);
            planar[i - 1] = glm::dvec2(glm::dot(q, u) / d, glm::dot(q, v) / d);
        }

        const Delaunay2D dt = delaunay2D(planar);

        const std::size_t hullSize = dt.hull.size();
        triangles.clear();
        triangles.reserve(dt.triangles.size() + 3 * hullSize);
        for (std::size_t t : dt.triangles) {
            triangles.push_back(static_cast<int>(t) + 1);
        }
        for (std::size_t k = 0; k < hullSize; k++) {
            triangles.push_back(0);
            triangles.push_back(static_cast<int>(dt.hull[k]) + 1);
            triangles.push_back(static_cast<int>(dt.hull[(k + 1) % hullSize]) + 1);
        }

        // Make every triangle counter clockwise seen from outside, and place its circumcentre.
        // Outside means away from the centroid, which is inside the hull even when the origin
        // isn't (a handful of points bunched on one side)
        glm::dvec3 centroid(0.0);
        for (const glm::vec3& p : points) centroid += glm::dvec3(p);
        centroid /= static_cast<double>(n);

        const std::size_t triCount = triangles.size() / 3;
        voronoi_points.resize(triCount);
        for (std::size_t t = 0; t < triCount; t++) {
            const glm::dvec3 a = glm::normalize(glm::dvec3(points[triangles[3 * t]]));
            const glm::dvec3 b = glm::normalize(glm::dvec3(points[triangles[3 * t + 1]]));
            const glm::dvec3 c = glm::normalize(glm::dvec3(points[triangles[3 * t + 2]]));
            glm::dvec3 normal = glm::cross(b - a, c - a);
            if (glm::dot(normal, a - centroid) < 0.0) {
                std::swap(triangles[3 * t + 1], triangles[3 * t + 2]);
                normal = -normal;
            }
            voronoi_points[t] = glm::vec3(glm::normalize(normal));
        }

        // Twins by looking up the reversed edge. A broken mesh leaves -1 here, validate() reports it
        auto key = [](int from, int to) {
            return (static_cast<std::uint64_t>(from) << 32) | static_cast<std::uint32_t>(to);
            };
        std::unordered_map<std::uint64_t, int> edgeIndex;
        edgeIndex.reserve(triangles.size());
        for (int e = 0, size = static_cast<int>(triangles.size()); e < size; e++) {
            edgeIndex[key(triangles[e], triangles[nextHalfedge(e)])] = e;
        }
        halfedges.assign(triangles.size(), -1);
        for (int e = 0, size = static_cast<int>(triangles.size()); e < size; e++) {
            auto it = edgeIndex.find(key(triangles[nextHalfedge(e)], triangles[e]));
            if (it != edgeIndex.end()) halfedges[e] = it->second;
        }
    }

    void SphereVoronoi::buildCells()
    {
        const int n = static_cast<int>(points.size());

        // Any halfedge ending at each point, to start its ring walk from
        std::vector<int> incoming(n, -1);
        for (int e = 0, size = static_cast<int>(triangles.size()); e < size; e++) {
            incoming[triangles[nextHalfedge(e)]] = e;
        }

        cells.clear();
        cells.reserve(n);
        cellLat.resize(n);
        cellLon.resize(n);
        for (int i = 0; i < n; i++) {
            cells.emplace_back(i);
            glm::vec2 latLon = unitToLatLon(points[i]);
            cellLat[i] = latLon.x;
            cellLon[i] = latLon.y;

            // Incoming halfedge a -> i of triangle k: a is neighbour k, and the twin's
            // previous halfedge is the incoming one of the next triangle counter clockwise
            const int start = incoming[i];
            if (start < 0) continue;
            int e = start;
            do {
                cells[i].vertex.push_back(e / 3);
                cells[i].neighbors.push_back(triangles[e]);
                const int twin = halfedges[e];
                if (twin < 0) break;
                e = prevHalfedge(twin);
            } while (e != start && cells[i].vertex.size() < triangles.size());
        }
    }

    namespace {

        // Signed area of the spherical triangle a, b, c; positive when counter clockwise from outside
        double sphericalTriangleArea(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c)
        {
            return 2.0 * std::atan2(glm::dot(a, glm::cross(b, c)), 1.0 + glm::dot(a, b) + glm::dot(b, c) + glm::dot(c, a));
        }

        // Counts failures per check and keeps the first example
        struct CheckLog {
            SphereValidation& out;
            std::ostringstream lines;
            std::unordered_map<std::string, int> counts;
            std::unordered_map<std::string, std::string> first;
            std::vector<std::string> order;

            explicit CheckLog(SphereValidation& v) : out(v) {}

            void fail(const std::string& check, const std::string& example) {
                out.ok = false;
                if (counts[check]++ == 0) {
                    first[check] = example;
                    order.push_back(check);
                }
            }

            void flush() {
                for (const std::string& check : order) {
                    lines << "FAIL " << check << ": " << counts[check] << "x, first: " << first[check] << "\n";
                }
            }
        };

        bool contains(const std::vector<int>& v, int x) {
            return std::find(v.begin(), v.end(), x) != v.end();
        }

        bool triangleHas(const std::vector<int>& triangles, int t, int p) {
            return triangles[3 * t] == p || triangles[3 * t + 1] == p || triangles[3 * t + 2] == p;
        }
    }

    SphereValidation validate(const SphereVoronoi& v)
    {
        SphereValidation result;
        CheckLog log(result);

        const int n = static_cast<int>(v.points.size());
        const int H = static_cast<int>(v.triangles.size());
        const int T = H / 3;

        if (H % 3 != 0 || static_cast<int>(v.halfedges.size()) != H || static_cast<int>(v.voronoi_points.size()) != T
            || static_cast<int>(v.cells.size()) != n || static_cast<int>(v.cellLat.size()) != n || static_cast<int>(v.cellLon.size()) != n) {
            log.fail("sizes", "array sizes disagree");
            log.flush();
            result.report = log.lines.str();
            return result;
        }

        // Twins: every halfedge has one, it points back and runs the other way
        for (int e = 0; e < H; e++) {
            const int t = v.halfedges[e];
            if (t < 0 || t >= H || t == e || v.halfedges[t] != e
                || v.triangles[t] != v.triangles[SphereVoronoi::nextHalfedge(e)]
                || v.triangles[SphereVoronoi::nextHalfedge(t)] != v.triangles[e]) {
                log.fail("twin halfedges", "halfedge " + std::to_string(e));
            }
        }

        // Euler characteristic of a sphere, with every point used
        std::vector<int> degree(n, 0);
        for (int e = 0; e < H; e++) degree[v.triangles[e]]++;
        int used = 0;
        for (int i = 0; i < n; i++) {
            if (degree[i] > 0) used++;
            else log.fail("unused points", "point " + std::to_string(i));
        }
        const int euler = used - H / 2 + T;
        if (euler != 2 || H % 2 != 0) {
            log.fail("Euler V - E + F", std::to_string(used) + " - " + std::to_string(H / 2) + " + " + std::to_string(T) + " = " + std::to_string(euler));
        }

        // Float points are only unit length to about 1e-7, which is enough to flip the verdict on
        // nearly cocircular points. Judge the same normalised doubles the triangulation used
        auto unit = [&](int i) { return glm::normalize(glm::dvec3(v.points[i])); };
        glm::dvec3 centroid(0.0);
        for (const glm::vec3& p : v.points) centroid += glm::dvec3(p);
        centroid /= static_cast<double>(std::max(n, 1));

        // Orientation, circumcentres and the empty circumcircle property
        double worstDelaunay = 0.0;
        for (int t = 0; t < T; t++) {
            const glm::dvec3 a = unit(v.triangles[3 * t]);
            const glm::dvec3 b = unit(v.triangles[3 * t + 1]);
            const glm::dvec3 c = unit(v.triangles[3 * t + 2]);
            const glm::dvec3 normal = glm::cross(b - a, c - a);
            if (glm::dot(normal, a - centroid) <= 0.0) {
                log.fail("triangle orientation", "triangle " + std::to_string(t));
            }

            const glm::dvec3 vp(v.voronoi_points[t]);
            const double ra = glm::dot(vp, a), rb = glm::dot(vp, b), rc = glm::dot(vp, c);
            if (std::abs(glm::length(vp) - 1.0) > 1e-5 || std::abs(ra - rb) > 1e-5 || std::abs(ra - rc) > 1e-5) {
                log.fail("circumcentre", "triangle " + std::to_string(t));
            }

            // Locally Delaunay everywhere means globally Delaunay: the far vertex of each
            // neighbouring triangle must not be inside this triangle's circumcircle
            const glm::dvec3 cc = glm::normalize(normal);
            const double radius = glm::dot(cc, a);
            for (int k = 0; k < 3; k++) {
                const int twin = v.halfedges[3 * t + k];
                if (twin < 0) continue;
                const glm::dvec3 far = unit(v.triangles[SphereVoronoi::prevHalfedge(twin)]);
                const double inside = glm::dot(cc, far) - radius;
                worstDelaunay = std::max(worstDelaunay, inside);
                if (inside > 1e-9) {
                    log.fail("empty circumcircle", "triangle " + std::to_string(t) + " by " + std::to_string(inside));
                }
            }
        }

        // Cells: closed counter clockwise rings with symmetric neighbours
        long long ringTotal = 0;
        double worstStep = std::numeric_limits<double>::max();
        const double stepTolerance = 1e-5 * 4.0 * glm::pi<double>() / std::max(n, 1);
        result.minRing = std::numeric_limits<int>::max();
        result.maxRing = 0;
        result.minCellArea = std::numeric_limits<double>::max();
        result.maxCellArea = 0.0;
        for (int i = 0; i < n; i++) {
            const Cell& cell = v.cells[i];
            const int ring = static_cast<int>(cell.vertex.size());
            ringTotal += ring;
            result.minRing = std::min(result.minRing, ring);
            result.maxRing = std::max(result.maxRing, ring);
            const std::string where = "cell " + std::to_string(i);

            if (cell.id != static_cast<unsigned int>(i)) log.fail("cell ids", where);
            if (ring < 3 || ring != degree[i] || static_cast<int>(cell.neighbors.size()) != ring) {
                log.fail("ring size", where + " has " + std::to_string(ring) + " vertices, degree " + std::to_string(degree[i]));
                continue;
            }

            const glm::dvec3 centre = unit(i);
            double area = 0.0;
            for (int k = 0; k < ring; k++) {
                const int t0 = cell.vertex[k];
                const int t1 = cell.vertex[(k + 1) % ring];
                const int nb = cell.neighbors[k];

                if (!triangleHas(v.triangles, t0, i)) log.fail("ring vertex not around cell", where);
                // The edge vertex[k] -> vertex[k + 1] is dual to the Delaunay edge i - neighbors[k]
                if (!triangleHas(v.triangles, t0, nb) || !triangleHas(v.triangles, t1, nb)) {
                    log.fail("neighbour not across its edge", where + " neighbour " + std::to_string(k));
                }
                if (!contains(v.cells[nb].neighbors, i)) log.fail("asymmetric neighbours", where + " and " + std::to_string(nb));
                for (int m = k + 1; m < ring; m++) {
                    if (cell.neighbors[m] == nb) log.fail("duplicate neighbour", where);
                }

                const double part = sphericalTriangleArea(centre, glm::dvec3(v.voronoi_points[t0]), glm::dvec3(v.voronoi_points[t1]));
                // Nearly cocircular points give circumcentres that round to the same float, so a
                // step can be zero or a rounding error below it. Real inversions are far bigger
                worstStep = std::min(worstStep, part);
                if (part < -stepTolerance) log.fail("ring not counter clockwise", where + " step " + std::to_string(k));
                area += part;
            }
            result.totalArea += area;
            result.minCellArea = std::min(result.minCellArea, area);
            result.maxCellArea = std::max(result.maxCellArea, area);

            const glm::vec3 back = latLonToUnit(v.cellLat[i], v.cellLon[i]);
            if (glm::length(back - v.points[i]) > 1e-5f) log.fail("cached lat/lon", where);
        }
        result.meanRing = n > 0 ? static_cast<double>(ringTotal) / n : 0.0;

        const double sphere = 4.0 * glm::pi<double>();
        const double areaError = std::abs(result.totalArea - sphere) / sphere;
        if (areaError > 1e-6) {
            log.fail("cell areas sum to 4 pi", "total " + std::to_string(result.totalArea) + ", relative error " + std::to_string(areaError));
        }

        log.flush();
        log.lines << "cells " << n << ", triangles " << T << ", ring size " << result.minRing << ".." << result.maxRing
            << " (mean " << result.meanRing << "), cell area ratio max/min " << result.maxCellArea / result.minCellArea
            << ", area error " << areaError << ", worst circumcircle intrusion " << worstDelaunay
            << ", smallest ring step area " << worstStep << "\n";
        result.report = log.lines.str();
        return result;
    }

}
