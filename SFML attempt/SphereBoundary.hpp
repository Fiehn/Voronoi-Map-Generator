#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "SphereVoronoi.hpp"
#include "RenderSettings.hpp"
#include "Ribbon.hpp"

// Smooth boundaries on the planet, the sphere port of the flat BoundaryRenderer.
// The edges between differently labelled cells are subdivided and smoothed with quadratic Bezier
// corner-cutting through the edge midpoints. The curves are built on the chords and pushed back
// onto the unit sphere. Render geometry only, the cells are never modified.
//
// The output is a rim per cell: its ring with the corners moved and the edge samples added,
// counter clockwise from outside like the ring. A point shared by two cells is the same float
// vec3 in both rims, so anything derived from it (lat/lon for the flat view) matches exactly.
// CPU only, so it can be checked without a window.
class SphereBoundaries
{
public:
    struct Polyline
    {
        BorderType type;
        bool closed;
        std::vector<glm::vec3> points; // Closed lines do not repeat the first point
    };

    // Call when the planet has been regenerated
    void invalidate();

    // Rebuilds the smoothed geometry if the planet, the labels or the settings changed.
    // Returns true when it did, so the render mesh has to be rebuilt
    bool sync(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode);

    bool valid() const { return geometryValid; }

    // Rim of cell i is rims[rimStart[i], rimStart[i + 1])
    const std::vector<glm::vec3>& rimPoints() const { return rims; }
    const std::vector<int>& rimOffsets() const { return rimStart; }
    const std::vector<Polyline>& boundaryLines() const { return polylines; }

    // Outlines of the boundary lines in their styles, lowest priority first so the coast is drawn
    // on top, and discs where several outlined lines end in the same vertex so junctions don't show notches
    void appendOutlines(RibbonBatch& batch, const RenderSettings& settings) const;

    // Debug views
    std::vector<glm::vec3> pinnedPoints() const;
    std::vector<glm::vec3> clampedPoints() const;
    std::vector<glm::vec3> originalEdges(const vor::SphereVoronoi& planet) const; // Boundary edges as pairs of points

    // Stats from the last rebuild
    unsigned int flips = 0;          // Flipped triangles left after clamping
    unsigned int clampedVertices = 0;
    unsigned int chainErrors = 0;
    double originalArea = 0.0;       // Of the cells, 4 pi on a valid planet
    double rimArea = 0.0;            // Of the fans from each cell centre to its rim
    float buildMs = 0.f;

private:
    struct Edge
    {
        int a, b; // Voronoi vertex ids (a < b)
        int cell[2] = { -1, -1 };
        bool forward[2] = { false, false }; // The cell ring walks the edge from a to b
        int count = 0; // Number of cell rings using this edge
        bool shared = false; // Used by exactly two cells that walk it in opposite directions
    };

    struct Chain
    {
        BorderType type;
        bool closed = false;
        std::vector<int> verts; // Closed chains do not repeat the first vertex
        std::vector<int> edges; // edges[i] connects verts[i] and verts[i + 1]
    };

    // Topology, only depends on the Voronoi geometry
    bool topologyValid = false;
    std::size_t topoCells = 0, topoVoronoiPoints = 0;
    double degenerateTolerance = 0.0; // Chord lengths, from the mean cell spacing
    double subdivideTolerance = 0.0;
    std::vector<Edge> edges;
    std::vector<int> ringStart; // Offset of each cell's ring into ringEdge/ringSign (size cells + 1)
    std::vector<int> ringEdge; // Edge index for ring position j (vertex[j] -> vertex[j + 1])
    std::vector<signed char> ringSign; // Winding sign of the original fan triangle, 0 when degenerate
    std::vector<char> vertexOnOpenEdge; // Vertex touches an edge that is not shared by exactly two cells
    std::vector<int> vertexCellStart, vertexCells; // Cells around each vertex (CSR)
    std::vector<glm::dvec3> vp; // voronoi_points in double

    // Geometry, depends on labels and settings
    bool geometryValid = false;
    uint64_t geometryKey = 0;
    std::vector<signed char> edgeType; // -1 for no boundary
    std::vector<char> pinned;
    std::vector<int> vertexNeighbors; // Two chain neighbours per smoothed vertex (-1 when unused)
    std::vector<float> vertexStrength;
    std::vector<float> vertexStrengthInitial;
    std::vector<glm::vec3> positions; // Smoothed vertex positions
    std::vector<int> sampleStart; // Per edge offset into samples (size edges + 1)
    std::vector<glm::vec3> samples; // Interior points of boundary edges, ordered from a to b
    std::vector<int> boundaryEdgeList; // Edges with a boundary type
    std::vector<int> smoothedVertices; // Vertices that move along a curve
    std::vector<int> affectedCells; // Cells with a moved vertex or a subdivided edge
    std::vector<char> cellAffected;
    std::vector<unsigned int> cellFlips; // Flipped triangles per cell from the last check
    std::vector<Chain> chains;
    std::vector<Polyline> polylines;
    std::vector<glm::vec3> rims;
    std::vector<int> rimStart;
    int firstFlipCell = -1; // For the debug log

    void buildTopology(const vor::SphereVoronoi& planet);
    void buildGeometry(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode);
    void buildChains();
    void computePositions();
    unsigned int collectFlips(const vor::SphereVoronoi& planet, const std::vector<int>& cells, std::vector<char>& offenders, unsigned int total);
    void buildRim(const vor::SphereVoronoi& planet, std::size_t cell, std::vector<glm::vec3>& rim, std::vector<int>& rimOwner, std::vector<int>& rimRing) const;
    void buildRims(const vor::SphereVoronoi& planet);
    void buildPolylines();
    uint64_t computeKey(const vor::SphereVoronoi& planet, const RenderSettings& settings, int mapMode) const;
};
