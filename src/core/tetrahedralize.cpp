// CyberMeshGenerator — public meshing entry points (foundation).
//
// The full incremental Bowyer-Watson Delaunay kernel and constrained-Delaunay
// meshing land in Phase 1 / Phase 3 as their own OpenSpec changes. The foundation
// wires the typed API end-to-end and implements the irreducible base case (four
// non-coplanar points -> one correctly-oriented tetrahedron) so the whole stack —
// options, robust predicates, Mesh — is exercised and testable now. Larger inputs
// return MeshErrorCode::NotImplemented rather than a wrong or partial mesh.
#include "cmg/api.hpp"

#include "cmg/constrained/tetrahedralize_plc.hpp"
#include "cmg/delaunay/incremental.hpp"
#include "cmg/predicates/robust.hpp"

namespace cmg {

namespace {

// Build the single tetrahedron spanning four non-coplanar points, oriented to
// TetGen's canonical sign (orient3d(v0,v1,v2,v3) < 0), with its four convex-hull
// faces marked 1.
Mesh single_tet(std::span<const Point3> p, IndexBase base) {
    Mesh m;
    m.index_base = base;
    m.points.assign(p.begin(), p.end());

    Tetrahedron t{0, 1, 2, 3};
    if (robust::orient3d(p[0], p[1], p[2], p[3]) > 0) {
        std::swap(t[2], t[3]); // flip to the canonical negative orientation
    }
    m.tetrahedra.push_back(t);
    m.tet_markers.push_back(0);

    // The four faces of the hull, each a triangle opposite one vertex.
    const int f[4][3] = {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
    for (const auto& tri : f) {
        m.faces.push_back({t[tri[0]], t[tri[1]], t[tri[2]]});
        m.face_markers.push_back(1);
    }
    m.point_markers.assign(p.size(), 0);
    return m;
}

} // namespace

expected<Mesh, MeshError> delaunay(std::span<const Point3> points,
                                   const MeshOptions& opts) {
    robust::ensure_initialized();

    if (points.size() < 4) {
        return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                    "a 3-D tetrahedralization needs at least 4 "
                                    "non-coplanar points"});
    }

    // Fast path for the irreducible unweighted 4-point case.
    if (points.size() == 4 && !opts.weighted) {
        if (robust::orient3d(points[0], points[1], points[2], points[3]) == 0) {
            return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                        "the four input points are coplanar"});
        }
        return single_tet(points, opts.index_base);
    }

    // General case: incremental Bowyer-Watson (Phase 1 kernel).
    return dt::incremental(points, opts);
}

expected<Mesh, MeshError> tetrahedralize(const PLC& in, const MeshOptions& opts) {
    robust::ensure_initialized();

    if (in.empty()) {
        return unexpected(
            MeshError{MeshErrorCode::InvalidInput, "the input PLC is empty"});
    }

    // A PLC carrying only points (no facets) is just a point-set Delaunay problem.
    if (in.facets.empty()) {
        return delaunay(in.points, opts);
    }

    // Faceted PLC: boundary-conforming tetrahedralization of the domain interior.
    return cdt::tetrahedralize_plc(in, opts);
}

} // namespace cmg
