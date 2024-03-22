#pragma once
#include <unordered_map>
#include <algorithm> 
#include <exception>
#include "cell.hpp"
#include "util.h"

namespace vor {

    constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();

    struct Grid { // Grid for spatial hashing
        std::size_t m_width;
		std::size_t m_height;
		std::vector<std::vector<std::vector<std::size_t>>> m_cells;
        Grid(std::size_t width, std::size_t height)
            : m_width(width), m_height(height), m_cells(width, std::vector<std::vector<std::size_t>>(height, std::vector<std::size_t>())) {}
        std::vector<std::size_t>& operator()(std::size_t x, std::size_t y) {
			return m_cells[x][y];
		}
        const std::vector<std::size_t>& operator()(std::size_t x, std::size_t y) const {
			return m_cells[x][y];
		}
        
        Grid() = default;
        
        void clear()
        {
            for (std::size_t i = 0; i < m_width; i++) {
                for (std::size_t j = 0; j < m_height; j++) {
					m_cells[i][j].clear();
				}
			}
		}
    };

    class BoolArray2D {
        private:
            bool* array;
            int width;
            int height;
        public:
            BoolArray2D(int width, int height) : width(width), height(height) {
			    array = new bool[width * height];
                for (int i = 0; i < width * height; i++) {
				    array[i] = false;
			    }
		    }
            ~BoolArray2D() {
                delete[] array;
                
            }
            bool& operator()(int row, int col) {
                return array[row * height + col];
            }
    };

    class Voronoi {
    public:
        std::vector<sf::Vector2f> points;
        std::vector<Cell> cells;
        std::vector<sf::Vector2f> voronoi_points; // deprecated ?
        std::vector<sf::Vertex> vertices;
        std::size_t vertexCount;
        vor::Grid grid_cells;
        int cell_size = 50;

        Voronoi(const int ncellx, const int ncelly, const int MAXWIDTH, const int MAXHEIGHT, const float jitter);

        int getCellIndex(sf::Vector2f point);

        ~Voronoi();

        void clearMap();

        void fillMap(const int ncellx, const int ncelly, const int MAXWIDTH, const int MAXHEIGHT, const float point_jitter);

    private:
        void generatePoints(const int ncellx, const int ncelly, const int MAXWIDTH, const int MAXHEIGHT, const float jitter);

        std::size_t getVertexCount();

        void vertexGen();

        void genGrid(const int MAXWIDTH, const int MAXHEIGHT);

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

        std::vector<std::size_t> delaunay();

        void voronoi(const std::vector<std::size_t> triangles);
    };

    Voronoi::Voronoi(const int ncellx, const int ncelly, const int MAXWIDTH, const int MAXHEIGHT, const float jitter)
    {

        generatePoints(ncellx, ncelly, MAXWIDTH, MAXHEIGHT, jitter);
        
        std::vector<std::size_t> triangles = delaunay();
        // Generate the Voronoi points after Delaunay triangulation is done
        voronoi(triangles);
        //Sort the verticies of each cell so they can be drawn
        for (std::size_t i = 0, size = cells.size(); i < size; i++) {
            if (cells[i].vertex.size() == 0) { continue; };
            cells[i].sort_angles(points, voronoi_points);
        }
        //Generate the vertices for the cells
        vertexGen();
        //Generate the grid
        genGrid(MAXWIDTH, MAXHEIGHT);
    }

    void Voronoi::fillMap(const int ncellx, const int ncelly, const int MAXWIDTH, const int MAXHEIGHT, const float point_jitter)
    {
        generatePoints(ncellx, ncelly, MAXWIDTH, MAXHEIGHT, point_jitter);
        std::vector<std::size_t> triangles = delaunay();
        voronoi(triangles);
        for (std::size_t i = 0, size = cells.size(); i < size; i++) {
			if (cells[i].vertex.size() == 0) { continue; };
			cells[i].sort_angles(points, voronoi_points);
		}
        vertexGen();
		genGrid(MAXWIDTH, MAXHEIGHT);
    }

    void Voronoi::genGrid(const int MAXWIDTH, const int MAXHEIGHT)
    {// Generate a grid that stores indices of cells that are inside the grid_cells vector
        grid_cells = vor::Grid(std::floor(MAXWIDTH / cell_size) + 1, std::floor(MAXHEIGHT / cell_size) + 1);
        
        for (int i = 0; i < cells.size(); i++)
        {
            vor::BoolArray2D bool_grid(std::floor(MAXWIDTH / cell_size) + 1, std::floor(MAXHEIGHT / cell_size) + 1);
            
            for (int j = 0; j < cells[i].vertex.size(); j++)
            {
                int x = std::floor(clamp_int(voronoi_points[cells[i].vertex[j]].x, MAXWIDTH, 0) / cell_size);
                int y = std::floor(clamp_int(voronoi_points[cells[i].vertex[j]].y, MAXHEIGHT, 0) / cell_size);
                // TODO: There are crashes and I suspect it's because of the grid_cells because they happen when the cells are being drawn
                if (bool_grid(x, y) == false)
                {
					grid_cells(x, y).push_back(i);
					bool_grid(x,y) = true;
				}
            }
        }
    }

    int Voronoi::getCellIndex(sf::Vector2f point)
    { // could be faster still than what the static grid can provide
        int grid_cell_x = point.x / cell_size;
        int grid_cell_y = point.y / cell_size;

        for (int i = 0; i < grid_cells(grid_cell_x, grid_cell_y).size(); i++)
        {
            int idx = grid_cells(grid_cell_x, grid_cell_y)[i];
            if (cells[idx].contains(point, voronoi_points))
            {
                return idx;
            }
        }

        return INVALID_INDEX;
	} 

    std::size_t Voronoi::getVertexCount()
    {
		std::size_t count = 0;
        for (std::size_t i = 0, size = cells.size(); i < size; i++) {
			count += cells[i].vertex.size();
		}
		return count;
	}

    void Voronoi::vertexGen()
    { // Collect all vertices for the triangles that draw the Voronoi map and store them in a vector (vertices)
        vertexCount = getVertexCount();
        vertices.reserve(vertexCount * 3);
        unsigned int offset = 0;
        for (std::size_t i = 0, size = cells.size(); i < size; i++) {
            if (cells[i].vertex.size() == 0) { continue; }
            else { cells[i].vertex_offset = offset; }

            for (std::size_t j = 0; j < cells[i].vertex.size(); j++)
            {
                vertices.push_back(sf::Vertex(voronoi_points[cells[i].vertex[j]], sf::Color::White));
                vertices.push_back(sf::Vertex(voronoi_points[cells[i].vertex[(j + 1) % cells[i].vertex.size()]], sf::Color::White));
                vertices.push_back(sf::Vertex(points[cells[i].id], sf::Color::White));
                
                offset += 3;
            }
        }
    }

    void Voronoi::clearMap()
    {
		points.clear();
		cells.clear();
		voronoi_points.clear();
		vertices.clear();
		grid_cells.clear();
	}

    Voronoi::~Voronoi()
    {
        points.clear();
        cells.clear();
        voronoi_points.clear();
        vertices.clear();
        grid_cells.clear();
    };

    void Voronoi::generatePoints(const int ncellx, const int ncelly, const int MAXWIDTH, const int MAXHEIGHT, const float jitter) 
    {
        float stepSizewidth = (float)MAXWIDTH / ncellx;
        float stepSizeHeight = (float)MAXHEIGHT / ncelly;

        for (int x = 0; x < MAXWIDTH; x = x + stepSizewidth) {
            for (int y = 0; y < MAXHEIGHT; y = y + stepSizeHeight) {
                sf::Vector2f p;
                sf::Vector2f random = randomGradient() * jitter;

                p.x = clamp(x + random.x, MAXWIDTH, 0);
                p.y = clamp(y + random.y, MAXHEIGHT, 0);
                points.push_back(p);

            }
        }
    }

    std::vector<std::size_t> Voronoi::delaunay()
    {
        std::size_t n = points.size();
        cells.reserve(n);

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
            cells.push_back(Cell(i)); // Need to do something about the cells
            // Stopped adding this here
        }
        // Center of the cells of points
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
        std::sort(ids.begin(), ids.end(), compare_dist_to_point{ points, m_center });

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
        m_hash[hash_key(points[i2], m_center, m_hash_size)] = i2;

        std::size_t max_triangles = n < 3 ? 1 : 2 * n - 5;
        triangles.reserve(max_triangles * 3);
        halfedges.reserve(max_triangles * 3);
        add_triangle(triangles, i0, i1, i2, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX, halfedges);

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
            size_t key = hash_key(points[i], m_center, m_hash_size);
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

            hull_tri[i] = legalize(t + 2, halfedges, hull_tri, hull_next, hull_start, m_edge_stack, triangles);
            hull_tri[e] = t;
            hull_size++;

            // walk forward through the hull, adding more triangles and flipping recursively
            std::size_t next = hull_next[e];
            while (
                q = hull_next[next],
                orient(points[i], points[next], points[q])) {
                t = add_triangle(triangles, next, i, q, hull_tri[i], INVALID_INDEX, hull_tri[next], halfedges);
                hull_tri[i] = legalize(t + 2, halfedges, hull_tri, hull_next, hull_start, m_edge_stack, triangles);
                hull_next[next] = next; // mark as removed
                hull_size--;
                next = q;
            }
            if (e == start) {
                while (
                    q = hull_prev[e],
                    orient(points[i], points[q], points[e])) {
                    t = add_triangle(triangles, q, i, e, INVALID_INDEX, hull_tri[e], hull_tri[q], halfedges);
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
        return triangles;
    }

    std::size_t Voronoi::legalize(std::size_t a, 
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

    inline std::size_t Voronoi::hash_key(
        sf::Vector2f p, 
        const sf::Vector2f& m_center, 
        const std::size_t& m_hash_size) const {
        const double dx = p.x - m_center.x;
        const double dy = p.y - m_center.y;
        return fast_mod(
            static_cast<std::size_t>(std::llround(std::floor(pseudo_angle(dx, dy) * static_cast<double>(m_hash_size)))),
            m_hash_size);
    }

    std::size_t Voronoi::add_triangle(
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

    void Voronoi::link(const std::size_t a, const std::size_t b, std::vector<std::size_t>& halfedges) {
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
    void Voronoi::voronoi(const std::vector<std::size_t> triangles) 
    {
        voronoi_points.reserve(triangles.size() / 3);
        for (int i = 0, j = 0,size = triangles.size(); i < size; i = i + 3, j++) {
            int i0 = triangles[i];
            int i1 = triangles[i + 1]; // Should be fixed
            int i2 = triangles[i + 2];

            // There should also be bounding boxes here
            // sf::Vector2f vor_point = circumcenter(points[i0], points[i1], points[i2]);
            voronoi_points.push_back(circumcenter(points[i0], points[i1], points[i2]));

            // add neighbors i0 takes i1 and i2 if they are not already in the vector
            insert_unique(cells[i0].neighbors, i1);
            insert_unique(cells[i0].neighbors, i2);
            insert_unique(cells[i1].neighbors, i0);
            insert_unique(cells[i1].neighbors, i2);
            insert_unique(cells[i2].neighbors, i1);
            insert_unique(cells[i2].neighbors, i0);

            // add voroni points to list of vertex
            cells[i0].vertex.push_back(j);
            cells[i1].vertex.push_back(j);
            cells[i2].vertex.push_back(j);
        }
    }


};



