// CyberMeshGenerator — background-mesh sizing function.
#pragma once

#include <functional>
#include <span>

#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"

namespace cmg::sizing {

/// Build a sizing function (target edge length at a point) from a background
/// tetrahedral mesh carrying a per-node size. The returned closure locates the
/// query point in `background` and barycentrically interpolates the node sizes,
/// multiplied by `scale`; a point outside the background mesh receives the nearest
/// node's size. `node_sizes.size()` must equal `background.points.size()`.
/// Suitable for `MeshOptions::sizing`. (TetGen -m background mesh / .mtr)
std::function<double(const Point3&)> from_background(
    const Mesh& background, std::span<const double> node_sizes,
    double scale = 1.0);

} // namespace cmg::sizing
