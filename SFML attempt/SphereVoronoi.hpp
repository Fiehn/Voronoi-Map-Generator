#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "cell.hpp"

namespace vor {

    // Fibonacci sphere points, each pushed along its tangent plane by up to
    // jitter * mean spacing per axis. Uses rand(), so the map seed applies.
    std::vector<glm::vec3> fibonacciPoints(int n, float jitter);

    // Voronoi cells on the unit sphere. Replaces vor::Voronoi once the simulation is ported.
    //
    // Delaunay mesh: triangles are counter clockwise seen from outside, and every halfedge
    // has a twin since the mesh is closed. Halfedge e runs from triangles[e] to
    // triangles[next(e)], triangle t owns halfedges 3t..3t+2.
    //
    // Cells: cells[i].vertex is the ring of voronoi_points around points[i], counter clockwise
    // seen from outside. cells[i].neighbors[k] is the cell across the edge from
    // vertex[k] to vertex[k + 1].
    class SphereVoronoi {
    public:
        std::vector<glm::vec3> points;         // Cell centres
        std::vector<Cell> cells;
        std::vector<glm::vec3> voronoi_points; // Circumcentre of each Delaunay triangle
        std::vector<float> cellLat;            // Degrees, cached from points
        std::vector<float> cellLon;

        std::vector<int> triangles;
        std::vector<int> halfedges;

        void fillMap(int n_cells, float point_jitter);
        void clearMap();

        // Cell containing the direction p (any length), or -1 if there are no cells. Walks to the
        // neighbour closest to p until none is closer, which on a Delaunay mesh ends at the
        // nearest centre. Starting from the previous answer makes repeated picks O(1)
        int findCell(const glm::vec3& p, int start = 0) const;

        static int nextHalfedge(int e) { return (e % 3 == 2) ? e - 2 : e + 1; }
        static int prevHalfedge(int e) { return (e % 3 == 0) ? e + 2 : e - 1; }

    private:
        void triangulate();
        void buildCells();
    };

    struct SphereValidation {
        bool ok = true;
        std::string report; // One line per failed check, then summary stats
        double totalArea = 0.0;
        int minRing = 0, maxRing = 0;
        double meanRing = 0.0;
        double minCellArea = 0.0, maxCellArea = 0.0;
    };

    // Checks the mesh is a closed, consistently oriented Delaunay triangulation of all points
    // (V - E + F = 2, twins, empty circumcircles) and that the cells are closed CCW rings with
    // symmetric neighbours whose areas sum to 4 pi. O(n), meant for debug builds and tests.
    SphereValidation validate(const SphereVoronoi& v);

}
