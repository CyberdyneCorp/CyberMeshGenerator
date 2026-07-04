// CyberMeshGenerator — mesh reconstruction (TetGen `-r`).
//
// Thin adapter over cmg::delaunay: re-tetrahedralize the input mesh's vertex set
// under the new options. delaunay() already applies opts.max_volume / opts.sizing
// refinement (inserting Steiner points as needed), so we simply forward its
// result. Re-using the vertex set preserves every input vertex and the convex
// domain those vertices span, while applying the finer constraints in `opts`.
#include "cmg/reconstruct/reconstruct.hpp"

#include "cmg/cmg.hpp"

namespace cmg::reconstruct {

expected<Mesh, MeshError> reconstruct(const Mesh& in, const MeshOptions& opts) {
    return cmg::delaunay(std::span<const Point3>(in.points), opts);
}

} // namespace cmg::reconstruct
