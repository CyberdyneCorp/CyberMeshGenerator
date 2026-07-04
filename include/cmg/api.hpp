// CyberMeshGenerator — public meshing entry points.
#pragma once

#include <span>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/options.hpp"
#include "cmg/core/plc.hpp"

namespace cmg {

/// Tetrahedralize a Piecewise-Linear Complex under the given options.
/// Returns the generated Mesh, or a MeshError describing a recoverable failure
/// (invalid PLC, self-intersection, unmet quality within budget, or a capability
/// not yet implemented). Never terminates the process.
expected<Mesh, MeshError> tetrahedralize(const PLC& in, const MeshOptions& opts);

/// Compute the Delaunay (or, with opts.weighted, regular) tetrahedralization of a
/// point set. Returns the Mesh or a MeshError.
expected<Mesh, MeshError> delaunay(std::span<const Point3> points,
                                   const MeshOptions& opts);

} // namespace cmg
