#include "Delaunator.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vor {

    namespace {

        constexpr std::size_t NONE = Delaunay2D::NONE;
        constexpr double EPS = std::numeric_limits<double>::epsilon();

        // Same predicates as util.hpp, in double

        inline double dist2(const glm::dvec2& a, const glm::dvec2& b) {
            const double dx = a.x - b.x;
            const double dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

        inline std::size_t fast_mod(const std::size_t i, const std::size_t c) {
            return i >= c ? i % c : i;
        }

        // monotonically increases with real angle
        inline double pseudo_angle(const double dx, const double dy) {
            const double p = dx / (std::abs(dx) + std::abs(dy));
            return (dy > 0.0 ? 3.0 - p : 1.0 + p) / 4.0; // [0..1)
        }

        inline double circumradius(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c) {
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
            return std::numeric_limits<double>::max();
        }

        inline bool orient(const glm::dvec2& p, const glm::dvec2& q, const glm::dvec2& r) {
            return (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y) < 0.0;
        }

        inline glm::dvec2 circumcenter(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c) {
            const double dx = b.x - a.x;
            const double dy = b.y - a.y;
            const double ex = c.x - a.x;
            const double ey = c.y - a.y;

            const double bl = dx * dx + dy * dy;
            const double cl = ex * ex + ey * ey;
            const double d = dx * ey - dy * ex;

            return glm::dvec2(a.x + (ey * bl - dy * cl) * 0.5 / d, a.y + (dx * cl - ex * bl) * 0.5 / d);
        }

        inline bool in_circle(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c, const glm::dvec2& p) {
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

        inline bool check_pts_equal(const glm::dvec2& a, const glm::dvec2& b) {
            return std::fabs(a.x - b.x) <= EPS && std::fabs(a.y - b.y) <= EPS;
        }

        // The working state of one triangulation, same names as the original Delaunator
        struct Builder {
            const std::vector<glm::dvec2>& points;
            std::vector<std::size_t> triangles;
            std::vector<std::size_t> halfedges;
            std::vector<std::size_t> hull_prev;
            std::vector<std::size_t> hull_next;
            std::vector<std::size_t> hull_tri;
            std::size_t hull_start = NONE;
            std::vector<std::size_t> m_hash;
            glm::dvec2 m_center;
            std::size_t m_hash_size = 0;
            std::vector<std::size_t> m_edge_stack;

            explicit Builder(const std::vector<glm::dvec2>& pts) : points(pts) {}

            std::size_t hash_key(const glm::dvec2& p) const {
                const double dx = p.x - m_center.x;
                const double dy = p.y - m_center.y;
                return fast_mod(
                    static_cast<std::size_t>(std::llround(std::floor(pseudo_angle(dx, dy) * static_cast<double>(m_hash_size)))),
                    m_hash_size);
            }

            void link(const std::size_t a, const std::size_t b) {
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
                if (b != NONE) {
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

            std::size_t add_triangle(std::size_t i0, std::size_t i1, std::size_t i2,
                std::size_t a, std::size_t b, std::size_t c) {
                std::size_t t = triangles.size();
                triangles.push_back(i0);
                triangles.push_back(i1);
                triangles.push_back(i2);
                link(t, a);
                link(t + 1, b);
                link(t + 2, c);
                return t;
            }

            std::size_t legalize(std::size_t a) {
                std::size_t i = 0;
                std::size_t ar = 0;
                m_edge_stack.clear();

                // recursion eliminated with a stack
                while (true) {
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
                    const std::size_t b = halfedges[a];
                    const std::size_t a0 = 3 * (a / 3);
                    ar = a0 + (a + 2) % 3;

                    if (b == NONE) {
                        if (i > 0) {
                            i--;
                            a = m_edge_stack[i];
                            continue;
                        }
                        break;
                    }

                    const std::size_t b0 = 3 * (b / 3);
                    const std::size_t al = a0 + (a + 1) % 3;
                    const std::size_t bl = b0 + (b + 2) % 3;

                    const std::size_t p0 = triangles[ar];
                    const std::size_t pr = triangles[a];
                    const std::size_t pl = triangles[al];
                    const std::size_t p1 = triangles[bl];

                    if (in_circle(points[p0], points[pr], points[pl], points[p1])) {
                        triangles[a] = p1;
                        triangles[b] = p0;

                        std::size_t hbl = halfedges[bl];

                        // edge swapped on the other side of the hull (rare); fix the halfedge reference
                        if (hbl == NONE) {
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
                        break;
                    }
                }
                return ar;
            }

            void run() {
                const std::size_t n = points.size();
                if (n < 3) throw std::runtime_error("not triangulation");

                double max_x = std::numeric_limits<double>::lowest();
                double max_y = std::numeric_limits<double>::lowest();
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
                }
                const glm::dvec2 c((min_x + max_x) / 2, (min_y + max_y) / 2);

                std::size_t i0 = NONE;
                std::size_t i1 = NONE;
                std::size_t i2 = NONE;

                // seed point closest to the centre
                double min_dist = std::numeric_limits<double>::max();
                for (std::size_t i = 0; i < n; i++) {
                    const double d = dist2(c, points[i]);
                    if (d < min_dist) {
                        i0 = i;
                        min_dist = d;
                    }
                }

                // find the point closest to the seed
                min_dist = std::numeric_limits<double>::max();
                for (std::size_t i = 0; i < n; i++) {
                    if (i == i0) continue;
                    const double d = dist2(points[i0], points[i]);
                    if (d < min_dist && d > 0.0) {
                        i1 = i;
                        min_dist = d;
                    }
                }
                if (i1 == NONE) throw std::runtime_error("not triangulation");

                // find the third point which forms the smallest circumcircle with the first two
                double min_radius = std::numeric_limits<double>::max();
                for (std::size_t i = 0; i < n; i++) {
                    if (i == i0 || i == i1) continue;
                    const double r = circumradius(points[i0], points[i1], points[i]);
                    if (r < min_radius) {
                        i2 = i;
                        min_radius = r;
                    }
                }
                if (!(min_radius < std::numeric_limits<double>::max())) {
                    throw std::runtime_error("not triangulation");
                }

                if (orient(points[i0], points[i1], points[i2])) {
                    std::swap(i1, i2);
                }

                m_center = circumcenter(points[i0], points[i1], points[i2]);

                // sort the points by distance from the seed triangle circumcenter, ties by x then y
                std::sort(ids.begin(), ids.end(), [&](std::size_t i, std::size_t j) {
                    const double diff1 = dist2(points[i], m_center) - dist2(points[j], m_center);
                    if (diff1 != 0.0) return diff1 < 0.0;
                    if (points[i].x != points[j].x) return points[i].x < points[j].x;
                    return points[i].y < points[j].y;
                    });

                // hash table for the edges of the advancing convex hull
                m_hash_size = static_cast<std::size_t>(std::llround(std::ceil(std::sqrt(n))));
                m_hash.assign(m_hash_size, NONE);

                hull_prev.resize(n);
                hull_next.resize(n);
                hull_tri.resize(n);

                hull_start = i0;

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
                add_triangle(i0, i1, i2, NONE, NONE, NONE);

                glm::dvec2 pp(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());

                for (std::size_t k = 0; k < n; k++) {
                    const std::size_t i = ids[k];

                    // skip near-duplicate points
                    if (k > 0 && check_pts_equal(points[i], pp)) continue;
                    pp = points[i];

                    // skip seed triangle points
                    if (check_pts_equal(points[i], points[i0]) ||
                        check_pts_equal(points[i], points[i1]) ||
                        check_pts_equal(points[i], points[i2])) continue;

                    // find a visible edge on the convex hull using edge hash
                    std::size_t start = 0;
                    std::size_t key = hash_key(points[i]);
                    for (std::size_t j = 0; j < m_hash_size; j++) {
                        start = m_hash[fast_mod(key + j, m_hash_size)];
                        if (start != NONE && start != hull_next[start]) break;
                    }
                    start = hull_prev[start];
                    std::size_t e = start;
                    std::size_t q;

                    while (q = hull_next[e], !orient(points[i], points[e], points[q])) {
                        e = q;
                        if (e == start) {
                            e = NONE;
                            break;
                        }
                    }
                    if (e == NONE) continue; // likely a near-duplicate point; skip it

                    // add the first triangle from the point
                    std::size_t t = add_triangle(e, i, hull_next[e], NONE, NONE, hull_tri[e]);

                    hull_tri[i] = legalize(t + 2);
                    hull_tri[e] = t;

                    // walk forward through the hull, adding more triangles and flipping recursively
                    std::size_t next = hull_next[e];
                    while (q = hull_next[next], orient(points[i], points[next], points[q])) {
                        t = add_triangle(next, i, q, hull_tri[i], NONE, hull_tri[next]);
                        hull_tri[i] = legalize(t + 2);
                        hull_next[next] = next; // mark as removed
                        next = q;
                    }

                    // walk backward from the other side, adding more triangles and flipping
                    if (e == start) {
                        while (q = hull_prev[e], orient(points[i], points[q], points[e])) {
                            t = add_triangle(q, i, e, NONE, hull_tri[e], hull_tri[q]);
                            legalize(t + 2);
                            hull_tri[q] = t;
                            hull_next[e] = e; // mark as removed
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
        };
    }

    Delaunay2D delaunay2D(const std::vector<glm::dvec2>& points)
    {
        Builder b(points);
        b.run();

        Delaunay2D result;
        result.triangles = std::move(b.triangles);
        result.halfedges = std::move(b.halfedges);

        std::size_t e = b.hull_start;
        do {
            result.hull.push_back(e);
            e = b.hull_next[e];
        } while (e != b.hull_start);

        return result;
    }

}
