// CyberMeshGenerator — incremental Bowyer-Watson Delaunay kernel (internal).
#pragma once

#include <span>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/options.hpp"

namespace cmg::dt {

/// Compute the Delaunay (or, with opts.weighted, regular) tetrahedralization of a
/// point set via incremental Bowyer-Watson insertion in BRIO-Hilbert order.
/// Assumes points.size() >= 4. Returns a MeshError on all-coplanar input.
expected<Mesh, MeshError> incremental(std::span<const Point3> points,
                                      const MeshOptions& opts);

} // namespace cmg::dt
