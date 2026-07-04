// CyberMeshGenerator — surface simplification (input PLC decimation).
//
// Rossignac–Borrel vertex clustering: reduce a triangulated PLC surface before
// meshing by collapsing nearby vertices to a grid-cell representative. This is a
// preprocessing utility on the INPUT surface — distinct from cmg::coarsen, which
// removes interior vertices from an already-built tetrahedral mesh.
#pragma once

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::simplify {

struct SimplifyOptions {
    /// Number of grid cells along the longest bounding-box axis. Higher = finer
    /// (more triangles retained); lower = coarser. Must be >= 1.
    int grid = 34;
};

/// Simplify a triangulated PLC surface by grid vertex clustering. Vertices sharing a
/// grid cell collapse to their centroid; facet triangles are re-emitted over the
/// representatives with degenerate and duplicate triangles dropped. Deterministic for
/// a given `grid`; the result has no more triangles than the input.
expected<PLC, MeshError> simplify(const PLC& in, const SimplifyOptions& opts = {});

} // namespace cmg::simplify
