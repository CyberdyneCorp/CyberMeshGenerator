// CyberMeshGenerator — Voronoi diagram (dual of the Delaunay tetrahedralization).
#pragma once

#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"

namespace cmg::voronoi {

/// A Voronoi edge: the finite segment between two Voronoi vertices, or — when
/// `v1 == -1` — a ray from `v0` along the outward unit direction `dir`.
struct Edge {
    int v0 = -1;
    int v1 = -1;      ///< -1 marks a ray to infinity
    Point3 dir{};     ///< outward unit direction, used only when v1 == -1
};

/// The Voronoi diagram dual to a Delaunay tetrahedralization.
struct VoronoiDiagram {
    std::vector<Point3> vertices;        ///< circumcenter per Delaunay tetrahedron
    std::vector<Edge> edges;             ///< one per Delaunay face
    std::vector<std::vector<int>> cells; ///< per input vertex: incident tet indices

    std::size_t ray_count() const {
        std::size_t n = 0;
        for (const Edge& e : edges) n += (e.v1 < 0);
        return n;
    }
};

/// Build the Voronoi diagram of a Delaunay mesh (tets stored orient3d(v0..v3) < 0).
VoronoiDiagram build(const Mesh& delaunay);

} // namespace cmg::voronoi
