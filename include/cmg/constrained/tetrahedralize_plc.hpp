// CyberMeshGenerator — boundary-conforming tetrahedralization of a PLC (internal).
#pragma once

#include <array>
#include <set>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/options.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::cdt {

/// Tetrahedralize the interior of the domain bounded by `plc`: the Delaunay
/// tetrahedralization of the PLC vertices, carved to the tetrahedra inside the
/// domain. Exact (volume-conserving) for convex / star-shaped domains; see the
/// change design for the deferred general boundary-recovery cases.
///
/// `constraint_faces` (optional) are recovered PLC facet subfaces (sorted vertex
/// triples). They are forwarded to region classification so a recovered internal
/// facet separates the regions on its two sides (see recover::recover_facets).
expected<Mesh, MeshError> tetrahedralize_plc(
    const PLC& plc, const MeshOptions& opts,
    const std::set<std::array<int, 3>>* constraint_faces = nullptr);

} // namespace cmg::cdt
