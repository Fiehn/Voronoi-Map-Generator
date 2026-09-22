#pragma once
#include <unordered_map>
#include <algorithm> 
#include <exception>
#include "cell.hpp"
#include "util.hpp"

namespace vor {

    constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();

    struct Grid { // For spacial partitioning, needs to be flattened further
        std::size_t m_width; // width of the grid in amount of gridcells
        std::size_t m_height; // height of the grid in amount of gridcells
        std::vector<std::vector<std::size_t>> m_cells; // 2D vector to store the indices of the cells in the grid

        Grid(std::size_t width, std::size_t height);
        std::vector<std::size_t>& operator()(std::size_t x, std::size_t y);
        const std::vector<std::size_t>& operator()(std::size_t x, std::size_t y) const;
        Grid() = default;
        void clear();
    };

    class BoolArray2D {
        private:
            bool* array;
            int width;
            int height;
        public:
            BoolArray2D(int width, int height);
            ~BoolArray2D();
            bool& operator()(int row, int col);
    };

    class Voronoi {
    public:
		std::vector<sf::Vector2f> points; // "Center"/initial points of the cells
		std::vector<Cell> cells; // Cells of the Voronoi map
        std::vector<sf::Vector2f> voronoi_points; // deprecated ?
		std::vector<sf::Vertex> vertices; // Vertices of the Voronoi map, to draw the cells
		std::size_t vertexCount; // Amount of vertices in the Voronoi map
		vor::Grid grid_cells; // Grid for spacial partitioning
		int cell_size = 50; // Size of the cells in the grid

        Voronoi();
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
    
}



