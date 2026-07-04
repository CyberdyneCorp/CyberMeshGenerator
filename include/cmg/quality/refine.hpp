// CyberMeshGenerator — Delaunay refinement for quality meshing (internal).
#pragma once

#include <span>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/options.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::quality {

/// Refine the mesh of the given vertex set by inserting circumcenter Steiner
/// points until the radius-edge and/or maximum-volume bounds in `opts` are met or
/// the Steiner budget is exhausted. When `plc` is non-null the domain is the carved
/// PLC interior; otherwise it is the convex hull of `points`. Reuses the Phase 1-3
/// kernels for re-meshing each round.
expected<Mesh, MeshError> refine(std::vector<Point3> points, const PLC* plc,
                                 const MeshOptions& opts);

} // namespace cmg::quality
