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
#include "util.h"

namespace Delu {

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

    constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();


    class Delaunator {
    public:
        std::vector<sf::Vector2f> const& points;
        
        std::vector<Cell> map;
        std::vector<sf::Vector2f> voronoi_points;

        Delaunator(std::vector<sf::Vector2f> const& in_points);

    private:
        std::size_t legalize(
            std::size_t a, 
            std::vector<std::size_t>& halfedges, 
            std::vector<std::size_t>& hull_tri, 
            std::vector<std::size_t>& hull_next, 
            const std::size_t& hull_start, 
            std::vector<std::size_t>& m_edge_stack,
            std::vector<std::size_t>& triangles);

        std::size_t hash_key(sf::Vector2f, const sf::Vector2f& m_center, const std::size_t& m_hash_size) const;
        std::size_t add_triangle(
            std::vector<std::size_t>& triangles,
            std::size_t i0,
            std::size_t i1,
            std::size_t i2,
            std::size_t a,
            std::size_t b,
            std::size_t c,
            std::vector<std::size_t>& halfedges);
        void link(std::size_t a, std::size_t b, std::vector<std::size_t>& halfedges);

        void voronoi(const std::vector<std::size_t>& triangles);
    };


    Delaunator::Delaunator(std::vector<sf::Vector2f> const& in_points) :
        points(in_points)
    {
        std::size_t n = points.size();

        std::vector<std::size_t> halfedges;
        std::vector<std::size_t> hull_prev;
        std::vector<std::size_t> hull_next;
        std::vector<std::size_t> hull_tri;
        std::size_t hull_start;
        std::vector<std::size_t> triangles;

        std::vector<std::size_t> m_hash;
        sf::Vector2f m_center;
        std::size_t m_hash_size;
        std::vector<std::size_t> m_edge_stack;

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
        m_hash[hash_key(points[i0], m_center, m_hash_size)] = i0;
        m_hash[hash_key(points[i1], m_center, m_hash_size)] = i1;
        m_hash[hash_key(points[i2], m_center,m_hash_size)] = i2;

        std::size_t max_triangles = n < 3 ? 1 : 2 * n - 5;
        triangles.reserve(max_triangles * 3);
        halfedges.reserve(max_triangles * 3);
        add_triangle(triangles, i0, i1, i2, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX,halfedges);

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
            size_t key = hash_key(points[i],m_center,m_hash_size);
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
                triangles,
                e,
                i,
                hull_next[e],
                INVALID_INDEX,
                INVALID_INDEX,
                hull_tri[e],
                halfedges);

            hull_tri[i] = legalize(t + 2, halfedges, hull_tri,hull_next,hull_start,m_edge_stack,triangles);
            hull_tri[e] = t;
            hull_size++;

            // walk forward through the hull, adding more triangles and flipping recursively
            std::size_t next = hull_next[e];
            while (
                q = hull_next[next],
                orient(points[i], points[next], points[q])) {
                t = add_triangle(triangles,next, i, q, hull_tri[i], INVALID_INDEX, hull_tri[next], halfedges);
                hull_tri[i] = legalize(t + 2, halfedges, hull_tri, hull_next, hull_start, m_edge_stack, triangles);
                hull_next[next] = next; // mark as removed
                hull_size--;
                next = q;
            }
            if (e == start) {
                while (
                    q = hull_prev[e],
                    orient(points[i], points[q], points[e])) {
                    t = add_triangle(triangles,q, i, e, INVALID_INDEX, hull_tri[e], hull_tri[q], halfedges);
                    legalize(t + 2, halfedges, hull_tri, hull_next, hull_start, m_edge_stack, triangles);
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

            m_hash[hash_key(points[i], m_center, m_hash_size)] = i;
            m_hash[hash_key(points[e], m_center, m_hash_size)] = e;
        }
        voronoi(triangles);
    }
    
    std::size_t Delaunator::legalize(std::size_t a, 
        std::vector<std::size_t>& halfedges, 
        std::vector<std::size_t>& hull_tri, 
        std::vector<std::size_t>& hull_next, 
        const std::size_t& hull_start, 
        std::vector<std::size_t>& m_edge_stack, 
        std::vector<std::size_t>& triangles) 
    {
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

            const bool illegal = in_circle(points[p0], points[pr], points[pl], points[p1]);

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
                link(a, hbl, halfedges);
                link(b, halfedges[ar], halfedges);
                link(ar, bl, halfedges);
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

    inline std::size_t Delaunator::hash_key(
        sf::Vector2f p, 
        const sf::Vector2f& m_center, 
        const std::size_t& m_hash_size) const {
        const double dx = p.x - m_center.x;
        const double dy = p.y - m_center.y;
        return fast_mod(
            static_cast<std::size_t>(std::llround(std::floor(pseudo_angle(dx, dy) * static_cast<double>(m_hash_size)))),
            m_hash_size);
    }

    std::size_t Delaunator::add_triangle(
        std::vector<std::size_t>& triangles, 
        std::size_t i0, 
        std::size_t i1, 
        std::size_t i2, 
        std::size_t a, 
        std::size_t b, 
        std::size_t c, 
        std::vector<std::size_t>& halfedges) 
    {
        std::size_t t = triangles.size();
        triangles.push_back(i0);
        triangles.push_back(i1);
        triangles.push_back(i2);
        link(t, a, halfedges);
        link(t + 1, b, halfedges);
        link(t + 2, c, halfedges);
        return t;
    }

    void Delaunator::link(const std::size_t a, const std::size_t b, std::vector<std::size_t>& halfedges) {
        std::size_t s = halfedges.size();
        if (a == s) {
            halfedges.push_back(b);
        }
        else if (a < s) {
            halfedges[a] = b;
        }
        if (a > s) {
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
    void Delaunator::voronoi(const std::vector<std::size_t>& triangles) 
    {

        int j = 0;
        for (int i = 0; i < triangles.size(); i = i + 3) {
            int i0 = triangles[i];
            int i1 = triangles[i + 1];
            int i2 = triangles[i + 2];

            // Calculate the voronoi point (you should check the rest of the conditions)
            // There should also be bounding boxes or whatever
            sf::Vector2f vor_point = circumcenter(points[i0], points[i1], points[i2]);
            voronoi_points.push_back(vor_point);

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

};



