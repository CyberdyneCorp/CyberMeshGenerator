// CyberMeshGenerator — mesh reconstruction (re-mesh / refine an existing mesh).
#pragma once

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/options.hpp"

namespace cmg::reconstruct {

/// Reconstruct/refine an existing tetrahedral mesh (TetGen `-r`): re-tetrahedralize
/// the mesh's vertex set and apply the refinement/sizing in `opts` (e.g. a new
/// `max_volume` or `sizing`), returning the new mesh. Lets a mesh loaded from
/// `.ele`/`.node` be refined to finer constraints. The vertex set — and hence the
/// convex domain it spans — is preserved; boundary vertices are retained.
expected<Mesh, MeshError> reconstruct(const Mesh& in, const MeshOptions& opts);

} // namespace cmg::reconstruct
