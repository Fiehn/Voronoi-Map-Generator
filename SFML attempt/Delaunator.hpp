#pragma once
#include <vector>
#include <cstddef>
#include <limits>
#include <glm/glm.hpp>

namespace vor {

    // Planar Delaunay triangulation in double precision, a port of Delaunator (mapbox/delaunator).
    // Used by the sphere Voronoi on stereographically projected points.
    struct Delaunay2D {
        static constexpr std::size_t NONE = std::numeric_limits<std::size_t>::max();

        std::vector<std::size_t> triangles; // 3 point indices per triangle
        std::vector<std::size_t> halfedges; // opposite halfedge, NONE on the hull
        std::vector<std::size_t> hull;      // convex hull point indices, in hull order
    };

    // Throws std::runtime_error if the points are all collinear or there are fewer than 3.
    // Exact and near duplicate points are skipped and end up in no triangle.
    Delaunay2D delaunay2D(const std::vector<glm::dvec2>& points);

}
