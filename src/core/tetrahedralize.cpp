// CyberMeshGenerator — public meshing entry points (foundation).
//
// The full incremental Bowyer-Watson Delaunay kernel and constrained-Delaunay
// meshing land in Phase 1 / Phase 3 as their own OpenSpec changes. The foundation
// wires the typed API end-to-end and implements the irreducible base case (four
// non-coplanar points -> one correctly-oriented tetrahedron) so the whole stack —
// options, robust predicates, Mesh — is exercised and testable now. Larger inputs
// return MeshErrorCode::NotImplemented rather than a wrong or partial mesh.
#include "cmg/api.hpp"

#include <algorithm>
#include <array>
#include <set>

#include "cmg/constrained/tetrahedralize_plc.hpp"
#include "cmg/delaunay/incremental.hpp"
#include "cmg/predicates/robust.hpp"
#include "cmg/quality/refine.hpp"
#include "cmg/recover/facets.hpp"
#include "cmg/recover/segments.hpp"

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

    const bool refine_requested =
        opts.quality.has_value() || opts.max_volume || opts.sizing;

    // Fast path for the irreducible unweighted 4-point case (no refinement).
    if (points.size() == 4 && !opts.weighted && !refine_requested) {
        if (robust::orient3d(points[0], points[1], points[2], points[3]) == 0) {
            return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                        "the four input points are coplanar"});
        }
        return single_tet(points, opts.index_base);
    }

    // Quality/size refinement (Phase 4) refines within the point set's hull.
    if (refine_requested && !opts.weighted) {
        return quality::refine({points.begin(), points.end()}, nullptr, opts);
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

    const std::size_t budget = opts.steiner_budget
                                   ? static_cast<std::size_t>(*opts.steiner_budget)
                                   : 100000;

    // Optional feature-edge recovery: augment the PLC's point set with Steiner
    // points so every facet edge appears as a chain of mesh edges, then mesh the
    // augmented PLC (facets unchanged; the Steiner points lie on facet edges).
    // Facet recovery implies edge recovery: recover facet-boundary edges first so
    // they exist before their interiors are recovered.
    PLC augmented;
    const PLC* work = &in;
    if (opts.preserve_edges || opts.preserve_facets) {
        auto segs = recover::facet_segments(in);
        auto rec = recover::recover_segments(in.points, segs, budget);
        augmented = in;
        augmented.points = std::move(rec.points);
        work = &augmented;
    }

    // Optional facet recovery: after segment recovery, add Steiner points until
    // every facet subface is a face of the Delaunay tetrahedralization, then treat
    // those subfaces as region-separating constraint faces during classification.
    // The seed/flood carve uses those constraint faces to separate an internal
    // facet's two sides, so general internal-facet region separation works whenever
    // recovery COMPLETES (all boundary subfaces are DT faces). We pass the constraint
    // faces to the carve only in that complete case: if recovery is incomplete a
    // subface is missing and the flood would leak through the gap, so we fall back to
    // the ray-cast carve (cf = nullptr), and region::apply likewise falls back to
    // merging across the un-recovered wall. Non-terminating recovery on flat coplanar
    // walls (cospherical points) remains a recovery limitation, not a carve one.
    std::set<std::array<int, 3>> constraint_faces;
    const std::set<std::array<int, 3>>* cf = nullptr;
    if (opts.preserve_facets) {
        auto subs = recover::plc_subfaces(*work);
        auto segs = recover::facet_segments(*work);
        auto fr = recover::recover_facets(work->points, subs, budget, segs);
        augmented.points = std::move(fr.points);
        work = &augmented;
        for (const auto& s : fr.subfaces) {
            std::array<int, 3> key{s[0], s[1], s[2]};
            std::sort(key.begin(), key.end());
            constraint_faces.insert(key);
        }
        if (fr.complete && !constraint_faces.empty()) cf = &constraint_faces;
    }

    // Faceted PLC: boundary-conforming tetrahedralization; refine if requested.
    if (opts.quality.has_value() || opts.max_volume || opts.sizing) {
        return quality::refine(work->points, work, opts);
    }
    return cdt::tetrahedralize_plc(*work, opts, cf);
}

} // namespace cmg
