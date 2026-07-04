// CyberMeshGenerator — region attributes and hole classification (internal).
#pragma once

#include "cmg/core/mesh.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::region {

/// Classify the carved interior mesh into connected components and apply the PLC's
/// regions and holes: remove components containing a hole seed, assign each region
/// seed's attribute to its component's tetrahedra (in Mesh::tet_markers), and —
/// when `label_regions` is set — give every surviving unseeded component a distinct
/// nonzero label. Recomputes the boundary faces afterward. A no-op when the PLC has
/// no regions or holes and `label_regions` is false.
void apply(Mesh& mesh, const PLC& plc, bool label_regions);

} // namespace cmg::region
