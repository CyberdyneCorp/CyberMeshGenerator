// CyberMeshGenerator — PLC self-intersection detection (TetGen -d).
//
// Flattens every facet polygon into a fan of triangles and reports pairs of
// non-adjacent triangles that genuinely cross. The triangle-triangle overlap
// test is the Guigue & Devillers orientation-predicate method ("Fast and Robust
// Triangle-Triangle Overlap Test Using Orientation Predicates", 2003): every
// decision is taken purely from the *sign* of an exact orient3d, so the test is
// robust and free of round-off. The interior branch structure mirrors the
// reference implementation's macros (CHECK_MIN_MAX / TRI_TRI_3D / the canonical
// permutation), which is why it is intentionally branch-heavy. Coplanar and
// merely-touching configurations are treated conservatively as "no crossing".
#include "cmg/detect/self_intersection.hpp"

#include <array>
#include <vector>

#include "cmg/predicates/robust.hpp"

namespace cmg::detect {

namespace {

using CP = const Point3&;

// Sign of the exact orient3d predicate: -1, 0, or +1. orient3d is exact, so the
// reported sign is reliable even for near-degenerate inputs (no NaN/inf).
int osign(CP a, CP b, CP c, CP d) {
    const double v = robust::orient3d(a, b, c, d);
    return (v > 0.0) - (v < 0.0);
}

// Guigue-Devillers CHECK_MIN_MAX, rewritten with orient3d sign tests. Returns
// true iff the two co-linear intersection intervals overlap.
bool check_min_max(CP p1, CP q1, CP r1, CP p2, CP q2, CP r2) {
    if (osign(q2, p2, p1, q1) > 0) return false;
    if (osign(r2, p2, r1, p1) > 0) return false;
    return true;
}

// Guigue-Devillers TRI_TRI_3D: both triangles already permuted into canonical
// form; (dp2,dq2,dr2) are the signs of triangle-2's vertices w.r.t. triangle-1's
// plane. A coplanar sub-case (all signs zero) is reported conservatively as no
// crossing.
bool tri_tri_3d(CP p1, CP q1, CP r1, CP p2, CP q2, CP r2, int dp2, int dq2,
                int dr2) {
    if (dp2 > 0) {
        if (dq2 > 0) return check_min_max(p1, r1, q1, r2, p2, q2);
        if (dr2 > 0) return check_min_max(p1, r1, q1, q2, r2, p2);
        return check_min_max(p1, q1, r1, p2, q2, r2);
    }
    if (dp2 < 0) {
        if (dq2 < 0) return check_min_max(p1, q1, r1, r2, p2, q2);
        if (dr2 < 0) return check_min_max(p1, q1, r1, q2, r2, p2);
        return check_min_max(p1, r1, q1, p2, q2, r2);
    }
    if (dq2 > 0) {
        if (dr2 >= 0) return check_min_max(p1, r1, q1, q2, r2, p2);
        return check_min_max(p1, q1, r1, p2, q2, r2);
    }
    if (dq2 < 0) {
        if (dr2 > 0) return check_min_max(p1, q1, r1, q2, r2, p2);
        return check_min_max(p1, r1, q1, p2, q2, r2);
    }
    if (dr2 > 0) return check_min_max(p1, q1, r1, r2, p2, q2);
    if (dr2 < 0) return check_min_max(p1, r1, q1, r2, p2, q2);
    return false; // fully coplanar — conservative
}

// Canonical permutation of triangle 1 by the signs of its vertices w.r.t.
// triangle 2's plane, then dispatch to tri_tri_3d. dp1<0 vs dp1>0 selects which
// permutation of triangle 2 keeps the orientation consistent.
bool overlap_pos(CP p1, CP q1, CP r1, CP p2, CP q2, CP r2, int dq1, int dr1,
                 int dp2, int dq2, int dr2) {
    if (dq1 > 0) return tri_tri_3d(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2);
    if (dr1 > 0) return tri_tri_3d(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2);
    return tri_tri_3d(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2);
}

bool overlap_neg(CP p1, CP q1, CP r1, CP p2, CP q2, CP r2, int dq1, int dr1,
                 int dp2, int dq2, int dr2) {
    if (dq1 < 0) return tri_tri_3d(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2);
    if (dr1 < 0) return tri_tri_3d(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2);
    return tri_tri_3d(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2);
}

bool overlap_zero(CP p1, CP q1, CP r1, CP p2, CP q2, CP r2, int dq1, int dr1,
                  int dp2, int dq2, int dr2) {
    if (dq1 < 0) {
        if (dr1 >= 0) return tri_tri_3d(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2);
        return tri_tri_3d(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2);
    }
    if (dq1 > 0) {
        if (dr1 > 0) return tri_tri_3d(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2);
        return tri_tri_3d(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2);
    }
    if (dr1 > 0) return tri_tri_3d(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2);
    if (dr1 < 0) return tri_tri_3d(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2);
    return false; // both triangles coplanar — conservative
}

// Guigue-Devillers top-level triangle-triangle overlap test. True iff the two
// triangles cross. The two early rejections (each plane fails to separate the
// other triangle's vertices) discard the vast majority of pairs cheaply.
bool tri_tri_overlap(CP p1, CP q1, CP r1, CP p2, CP q2, CP r2) {
    const int dp1 = osign(p1, p2, q2, r2);
    const int dq1 = osign(q1, p2, q2, r2);
    const int dr1 = osign(r1, p2, q2, r2);
    if (dp1 * dq1 > 0 && dp1 * dr1 > 0) return false;

    const int dp2 = osign(p2, q1, r1, p1);
    const int dq2 = osign(q2, q1, r1, p1);
    const int dr2 = osign(r2, q1, r1, p1);
    if (dp2 * dq2 > 0 && dp2 * dr2 > 0) return false;

    if (dp1 > 0)
        return overlap_pos(p1, q1, r1, p2, q2, r2, dq1, dr1, dp2, dq2, dr2);
    if (dp1 < 0)
        return overlap_neg(p1, q1, r1, p2, q2, r2, dq1, dr1, dp2, dq2, dr2);
    return overlap_zero(p1, q1, r1, p2, q2, r2, dq1, dr1, dp2, dq2, dr2);
}

// A flattened facet triangle: its three vertex indices (as given in the PLC) and
// the facet it originated from.
struct FlatTri {
    std::array<int, 3> v{};
    int facet = -1;
};

bool shares_vertex(const std::array<int, 3>& a, const std::array<int, 3>& b) {
    for (int x : a)
        for (int y : b)
            if (x == y) return true;
    return false;
}

} // namespace

std::vector<FacetIntersection> self_intersections(const PLC& plc) {
    robust::ensure_initialized();

    const int base = (plc.index_base == IndexBase::One) ? 1 : 0;
    const int npts = static_cast<int>(plc.points.size());

    // Flatten every facet polygon into a fan of triangles, in facet order.
    std::vector<FlatTri> tris;
    for (int f = 0; f < static_cast<int>(plc.facets.size()); ++f) {
        for (const Polygon& poly : plc.facets[f].polygons) {
            const std::vector<Index>& vs = poly.vertices;
            for (std::size_t i = 2; i < vs.size(); ++i) {
                tris.push_back(
                    FlatTri{{vs[0], vs[i - 1], vs[i]}, f});
            }
        }
    }

    // A triangle's stored index i is valid iff (i - base) is in [0, npts).
    auto valid = [&](const std::array<int, 3>& t) {
        for (int idx : t) {
            const int p = idx - base;
            if (p < 0 || p >= npts) return false;
        }
        return true;
    };
    auto pt = [&](int idx) -> const Point3& { return plc.points[idx - base]; };

    std::vector<FacetIntersection> out;
    const int n = static_cast<int>(tris.size());
    for (int i = 0; i < n; ++i) {
        if (!valid(tris[i].v)) continue;
        for (int j = i + 1; j < n; ++j) {
            // Adjacent triangles sharing a vertex/edge are not intersections.
            if (shares_vertex(tris[i].v, tris[j].v)) continue;
            if (!valid(tris[j].v)) continue;

            const Point3& a0 = pt(tris[i].v[0]);
            const Point3& a1 = pt(tris[i].v[1]);
            const Point3& a2 = pt(tris[i].v[2]);
            const Point3& b0 = pt(tris[j].v[0]);
            const Point3& b1 = pt(tris[j].v[1]);
            const Point3& b2 = pt(tris[j].v[2]);

            if (tri_tri_overlap(a0, a1, a2, b0, b1, b2)) {
                out.push_back(FacetIntersection{tris[i].facet, tris[j].facet,
                                                tris[i].v, tris[j].v});
            }
        }
    }
    return out;
}

} // namespace cmg::detect
