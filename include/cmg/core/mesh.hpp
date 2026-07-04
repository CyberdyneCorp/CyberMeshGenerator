// CyberMeshGenerator — the output tetrahedral mesh.
//
// Replaces the raw output arrays of TetGen's `tetgenio` with owning, RAII
// containers. Optional adjacency and second-order data are opt-in (populated only
// when the corresponding options request them).
#pragma once

#include <vector>

#include "cmg/core/geometry.hpp"

namespace cmg {

/// An owning tetrahedral mesh: vertices, tetrahedra, boundary faces, and optional
/// markers / adjacency. Move-cheap; releases all storage by RAII.
class Mesh {
public:
    std::vector<Point3> points;
    std::vector<Tetrahedron> tetrahedra;
    std::vector<Triangle> faces;

    std::vector<int> point_markers;
    std::vector<int> tet_markers;  ///< region/material attribute per tetrahedron
    std::vector<int> face_markers; ///< boundary marker per face

    // Opt-in adjacency (analogue of TetGen's -n / -nn output). Empty unless
    // requested via MeshOptions.
    std::vector<Tetrahedron> neighbors; ///< 4 tet-neighbor indices, -1 for hull

    IndexBase index_base = IndexBase::Zero;

    std::size_t point_count() const noexcept { return points.size(); }
    std::size_t tet_count() const noexcept { return tetrahedra.size(); }
    std::size_t face_count() const noexcept { return faces.size(); }

    bool empty() const noexcept { return points.empty(); }
};

} // namespace cmg
