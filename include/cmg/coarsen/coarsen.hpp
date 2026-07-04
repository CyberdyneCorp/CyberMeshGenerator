// CyberMeshGenerator — mesh coarsening (vertex removal / decimation).
#pragma once

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/mesh.hpp"

namespace cmg::coarsen {

struct CoarsenOptions {
    /// Fraction of interior (non-boundary) vertices to KEEP, in (0, 1]. 0.5 removes
    /// roughly half of the interior vertices.
    double keep_fraction = 0.5;
    unsigned long long seed = 1; ///< deterministic selection of which to remove
};

/// Coarsen a tetrahedral mesh by removing a fraction of its interior vertices and
/// re-tetrahedralizing the remaining vertices (TetGen `-R`). Boundary vertices
/// (those on any boundary face) are always kept, so the domain is preserved.
/// Reduces element count while remaining a valid Delaunay tetrahedralization.
expected<Mesh, MeshError> coarsen(const Mesh& in, const CoarsenOptions& opts = {});

} // namespace cmg::coarsen
