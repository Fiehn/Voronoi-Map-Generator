#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>
#include "points.hpp"
#include "cell.hpp"


// stolen and repurposed from:
// https://github.com/delfrrr/delaunator-cpp/blob/master/include/delaunator.hpp

inline size_t fast_mod(const size_t i, const size_t c) {
    return i >= c ? i % c : i;
}

// Kahan and Babuska summation, Neumaier variant; accumulates less FP error
inline double sum(const std::vector<double>& x) {
    double sum = x[0];
    double err = 0.0;

    for (int i = 1; i < x.size(); i++) {
        const double k = x[i];
        const double m = sum + k;
        err += std::fabs(sum) >= std::fabs(k) ? sum - m + k : k - m + sum;
        sum = m;
    }
    return sum + err;
}

inline double circumradius(sf::Vector2f a,sf::Vector2f b,sf::Vector2f c) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double ex = c.x - a.x;
    const double ey = c.y - a.y;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = (ey * bl - dy * cl) * 0.5 / d;
    const double y = (dx * cl - ex * bl) * 0.5 / d;

    if ((bl > 0.0 || bl < 0.0) && (cl > 0.0 || cl < 0.0) && (d > 0.0 || d < 0.0)) {
        return x * x + y * y;
    }
    else {
        return std::numeric_limits<double>::max();
    }
}

inline bool orient(sf::Vector2f p,sf::Vector2f q,sf::Vector2f r) {
    return (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y) < 0.0;
}

inline sf::Vector2f circumcenter(sf::Vector2f a, sf::Vector2f b,sf::Vector2f c) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double ex = c.x - a.x;
    const double ey = c.y - a.y;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = a.x + (ey * bl - dy * cl) * 0.5 / d;
    const double y = a.y + (dx * cl - ex * bl) * 0.5 / d;

    return sf::Vector2f(x,y);
}

struct compare {

    std::vector<sf::Vector2f> const& points;
    sf::Vector2f p;

    bool operator()(std::size_t i, std::size_t j) {
        const double d1 = dist(points[i], p);
        const double d2 = dist(points[j], p);
        const double diff1 = d1 - d2;
        const double diff2 = points[i].x - points[j].x;
        const double diff3 = points[i].y - points[j].y;

        if (diff1 > 0.0 || diff1 < 0.0) {
            return diff1 < 0;
        }
        else if (diff2 > 0.0 || diff2 < 0.0) {
            return diff2 < 0;
        }
        else {
            return diff3 < 0;
        }
    }
};

inline bool in_circle(sf::Vector2f a,sf::Vector2f b, sf::Vector2f c,sf::Vector2f p) {
    const double dx = a.x - p.x;
    const double dy = a.y - p.y;
    const double ex = b.x - p.x;
    const double ey = b.y - p.y;
    const double fx = c.x - p.x;
    const double fy = c.y - p.y;

    const double ap = dx * dx + dy * dy;
    const double bp = ex * ex + ey * ey;
    const double cp = fx * fx + fy * fy;

    return (dx * (ey * cp - bp * fy) -
        dy * (ex * cp - bp * fx) +
        ap * (ex * fy - ey * fx)) < 0.0;
}

constexpr double EPSILON = std::numeric_limits<double>::epsilon();
constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();

inline bool check_pts_equal(sf::Vector2f a,sf::Vector2f b) {
    return std::fabs(a.x - b.x) <= EPSILON &&
        std::fabs(a.y - b.y) <= EPSILON;
}

struct DelaunatorPoint {
    std::size_t i;
    double x;
    double y;
    std::size_t t;
    std::size_t prev;
    std::size_t next;
    bool removed;
};

class Delaunator {
public:
    std::vector<sf::Vector2f> const& points;
    std::vector<std::size_t> triangles;
    std::vector<std::size_t> halfedges;
    std::vector<std::size_t> hull_prev;
    std::vector<std::size_t> hull_next;
    std::vector<std::size_t> hull_tri;
    std::size_t hull_start;

    Delaunator(std::vector<sf::Vector2f> const& in_points);


    double get_hull_area();

private:
    std::vector<std::size_t> m_hash;
    sf::Vector2f m_center;
    std::size_t m_hash_size;
    std::vector<std::size_t> m_edge_stack;

    std::size_t legalize(std::size_t a);
    std::size_t hash_key(sf::Vector2f) const;
    std::size_t add_triangle(
        std::size_t i0,
        std::size_t i1,
        std::size_t i2,
        std::size_t a,
        std::size_t b,
        std::size_t c);
    void link(std::size_t a, std::size_t b);
};

std::vector<Cell> map;

Delaunator::Delaunator(std::vector<sf::Vector2f> const& in_points): 
    points(in_points),
    triangles(),
    halfedges(),
    hull_prev(),
    hull_next(),
    hull_tri(),
    hull_start(),
    m_hash(),
    m_center(),
    m_hash_size(),
    m_edge_stack() {
    //std::size_t n = points.size() >> 1; why was this here??
    std::size_t n = points.size();

    double max_x = std::numeric_limits<double>::min();
    double max_y = std::numeric_limits<double>::min();
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    std::vector<std::size_t> ids;
    ids.reserve(n);

    for (std::size_t i = 0; i < n; i++) {
        const double x = points[i].x;
        const double y = points[i].y;

        if (x < min_x) min_x = x;
        if (y < min_y) min_y = y;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;

        ids.push_back(i);
        // Added this myself!!!
        map.push_back(Cell(i)); // Need to do something about the map
        // Stopped adding this here
    }
    // Center of the map of points
    sf::Vector2f c((min_x + max_x) / 2, (min_y + max_y) / 2);
    // Initialize the smallest distance from the center
    double min_dist = std::numeric_limits<double>::max(); 

    // Initialize index
    std::size_t i0 = INVALID_INDEX;
    std::size_t i1 = INVALID_INDEX;
    std::size_t i2 = INVALID_INDEX;

    for (std::size_t i = 0; i < n; i++) {
        const double d = dist(c, points[i]);
        // Finds the point with the smallest distance to the center (seed)
        if (d < min_dist) {
            i0 = i;
            min_dist = d;
        }
    }

    min_dist = std::numeric_limits<double>::max();

    // find the point closest to the seed
    for (std::size_t i = 0; i < n; i++) {
        if (i == i0) continue;
        const double d = dist(points[i0], points[i]);
        if (d < min_dist && d > 0.0) {
            i1 = i;
            min_dist = d;
        }
    }

    double min_radius = std::numeric_limits<double>::max();

    // find the third point which forms the smallest circumcircle with the first two
    for (std::size_t i = 0; i < n; i++) {
        if (i == i0 || i == i1) continue;
        const double r = circumradius(points[i0], points[i1], points[i]);
        if (r < min_radius) {
            i2 = i;
            min_radius = r;
        }
    }
    // If its not possible
    if (!(min_radius < std::numeric_limits<double>::max())) {
        throw std::runtime_error("not triangulation");
    }

    if (orient(points[i0], points[i1], points[i2])) {
        std::swap(i1, i2);
    }

    m_center = circumcenter(points[i0], points[i1], points[i2]);

    // sort the points by distance from the seed triangle circumcenter
    std::sort(ids.begin(), ids.end(), compare{ points, m_center });

    // initialize a hash table for storing edges of the advancing convex hull
    // takes the sqrt of n then ceiling then round then cast it into data type size_t
    m_hash_size = static_cast<std::size_t>(std::llround(std::ceil(std::sqrt(n))));
    m_hash.resize(m_hash_size);
    std::fill(m_hash.begin(), m_hash.end(), INVALID_INDEX);

    // initialize arrays for tracking the edges of the advancing convex hull
    hull_prev.resize(n);
    hull_next.resize(n);
    hull_tri.resize(n);

    hull_start = i0; // Start at t´he seed

    size_t hull_size = 3;
    
    // add the first triangle
    hull_next[i0] = hull_prev[i2] = i1;
    hull_next[i1] = hull_prev[i0] = i2;
    hull_next[i2] = hull_prev[i1] = i0;

    hull_tri[i0] = 0;
    hull_tri[i1] = 1;
    hull_tri[i2] = 2;
    m_hash[hash_key(points[i0])] = i0;
    m_hash[hash_key(points[i1])] = i1;
    m_hash[hash_key(points[i2])] = i2;

    std::size_t max_triangles = n < 3 ? 1 : 2 * n - 5;
    triangles.reserve(max_triangles * 3);
    halfedges.reserve(max_triangles * 3);
    add_triangle(i0, i1, i2, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX);

    // Potential bug territory!
    sf::Vector2f pp(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());

    for (std::size_t k = 0; k < n; k++) {
        const std::size_t i = ids[k];

        // skip near-duplicate points
        if (k > 0 && check_pts_equal(points[i], pp)) continue;
        pp = points[i];

        // skip seed triangle points
        if (
            check_pts_equal(points[i], points[i0]) ||
            check_pts_equal(points[i], points[i1]) ||
            check_pts_equal(points[i], points[i2])) continue;

        // find a visible edge on the convex hull using edge hash
        std::size_t start = 0;

        // Bug search here!
        size_t key = hash_key(points[i]);
        for (size_t j = 0; j < m_hash_size; j++) {
            start = m_hash[fast_mod(key + j, m_hash_size)];
            if (start != INVALID_INDEX && start != hull_next[start]) break;
        }
        start = hull_prev[start];
        size_t e = start;
        size_t q;

        while (q = hull_next[e], !orient(points[i], points[e], points[q])) { //TODO: does it works in a same way as in JS
            e = q;
            if (e == start) {
                e = INVALID_INDEX;
                break;
            }
        }
        if (e == INVALID_INDEX) continue; // likely a near-duplicate point; skip it

        // add the first triangle from the point
        std::size_t t = add_triangle(
            e,
            i,
            hull_next[e],
            INVALID_INDEX,
            INVALID_INDEX,
            hull_tri[e]);

        hull_tri[i] = legalize(t + 2);
        hull_tri[e] = t;
        hull_size++;

        // walk forward through the hull, adding more triangles and flipping recursively
        std::size_t next = hull_next[e];
        while (
            q = hull_next[next],
            orient(points[i], points[next], points[q])) {
            t = add_triangle(next, i, q, hull_tri[i], INVALID_INDEX, hull_tri[next]);
            hull_tri[i] = legalize(t + 2);
            hull_next[next] = next; // mark as removed
            hull_size--;
            next = q;
        }
        if (e == start) {
            while (
                q = hull_prev[e],
                orient(points[i], points[q], points[e])) {
                t = add_triangle(q, i, e, INVALID_INDEX, hull_tri[e], hull_tri[q]);
                legalize(t + 2);
                hull_tri[q] = t;
                hull_next[e] = e; // mark as removed
                hull_size--;
                e = q;
            }
        }

        // update the hull indices
        hull_prev[i] = e;
        hull_start = e;
        hull_prev[next] = i;
        hull_next[e] = i;
        hull_next[i] = next;

        m_hash[hash_key(points[i])] = i;
        m_hash[hash_key(points[e])] = e;
    }
}

double Delaunator::get_hull_area() {
    std::vector<double> hull_area;
    size_t e = hull_start;
    do {
        hull_area.push_back((points[e].x - points[hull_prev[e]].x) * (points[e].y + points[hull_prev[e]].y));
        e = hull_next[e];
    } while (e != hull_start);
    return sum(hull_area);
}

std::size_t Delaunator::legalize(std::size_t a) {
    std::size_t i = 0;
    std::size_t ar = 0;
    m_edge_stack.clear();

    // recursion eliminated with a fixed-size stack
    while (true) {
        const size_t b = halfedges[a];

        /* if the pair of triangles doesn't satisfy the Delaunay condition
        * (p1 is inside the circumcircle of [p0, pl, pr]), flip them,
        * then do the same check/flip recursively for the new pair of triangles
        *
        *           pl                    pl
        *          /||\                  /  \
        *       al/ || \bl            al/    \a
        *        /  ||  \              /      \
        *       /  a||b  \    flip    /___ar___\
        *     p0\   ||   /p1   =>   p0\---bl---/p1
        *        \  ||  /              \      /
        *       ar\ || /br             b\    /br
        *          \||/                  \  /
        *           pr                    pr
        */
        const size_t a0 = 3 * (a / 3);
        ar = a0 + (a + 2) % 3;

        if (b == INVALID_INDEX) {
            if (i > 0) {
                i--;
                a = m_edge_stack[i];
                continue;
            }
            else {
                //i = INVALID_INDEX;
                break;
            }
        }

        const size_t b0 = 3 * (b / 3);
        const size_t al = a0 + (a + 1) % 3;
        const size_t bl = b0 + (b + 2) % 3;

        const std::size_t p0 = triangles[ar];
        const std::size_t pr = triangles[a];
        const std::size_t pl = triangles[al];
        const std::size_t p1 = triangles[bl];

        const bool illegal = in_circle(points[p0],points[pr],points[pl],points[p1]);

        if (illegal) {
            triangles[a] = p1;
            triangles[b] = p0;

            auto hbl = halfedges[bl];

            // edge swapped on the other side of the hull (rare); fix the halfedge reference
            if (hbl == INVALID_INDEX) {
                std::size_t e = hull_start;
                do {
                    if (hull_tri[e] == bl) {
                        hull_tri[e] = a;
                        break;
                    }
                    e = hull_next[e];
                } while (e != hull_start);
            }
            link(a, hbl);
            link(b, halfedges[ar]);
            link(ar, bl);
            std::size_t br = b0 + (b + 1) % 3;

            if (i < m_edge_stack.size()) {
                m_edge_stack[i] = br;
            }
            else {
                m_edge_stack.push_back(br);
            }
            i++;

        }
        else {
            if (i > 0) {
                i--;
                a = m_edge_stack[i];
                continue;
            }
            else {
                break;
            }
        }
    }
    return ar;
}

inline std::size_t Delaunator::hash_key(sf::Vector2f p) const {
    const double dx = p.x - m_center.x;
    const double dy = p.y - m_center.y;
    return fast_mod(
        static_cast<std::size_t>(std::llround(std::floor(pseudo_angle(dx, dy) * static_cast<double>(m_hash_size)))),
        m_hash_size);
}

std::size_t Delaunator::add_triangle(std::size_t i0,std::size_t i1,std::size_t i2,std::size_t a,std::size_t b,std::size_t c) {
    std::size_t t = triangles.size();
    triangles.push_back(i0);
    triangles.push_back(i1);
    triangles.push_back(i2);
    link(t, a);
    link(t + 1, b);
    link(t + 2, c);
    return t;
}

void Delaunator::link(const std::size_t a, const std::size_t b) {
    std::size_t s = halfedges.size();
    if (a == s) {
        halfedges.push_back(b);
    }
    else if (a < s) {
        halfedges[a] = b;
    }
    else {
        throw std::runtime_error("Cannot link edge");
    }
    if (b != INVALID_INDEX) {
        std::size_t s2 = halfedges.size();
        if (b == s2) {
            halfedges.push_back(a);
        }
        else if (b < s2) {
            halfedges[b] = a;
        }
        else {
            throw std::runtime_error("Cannot link edge");
        }
    }
}

// fix the things
void voroi(Delaunator d) {
    int j = 0;
    for (int i = 0; i < d.triangles.size(); i = i + 3) {
        int i0 = d.triangles[i];
        int i1 = d.triangles[i + 1];
        int i2 = d.triangles[i + 2];

        // Calculate the voronoi point (you should check the rest of the conditions)
        // There should also be bounding boxes or whatever
        sf::Vector2f vor_point = circumcenter(points[i0], points[i1], points[i2]);
        voroi_points.push_back(vor_point);

        // add neighbors i0 takes i1 and i2 if they are not already in the vector
        map[i0].add_neighbors(i1);
        map[i0].add_neighbors(i2);
        map[i1].add_neighbors(i0);
        map[i1].add_neighbors(i2);
        map[i2].add_neighbors(i1);
        map[i2].add_neighbors(i0);
        
        // add voroni points to list of vertex
        map[i0].add_vertex(j);
        map[i1].add_vertex(j);
        map[i2].add_vertex(j);

        j++;
    }
}





