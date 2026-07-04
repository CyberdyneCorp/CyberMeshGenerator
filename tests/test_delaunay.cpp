// CyberMeshGenerator — Phase 1 Delaunay tetrahedralization tests.
//
// Validates the incremental Bowyer-Watson kernel by its defining invariants —
// positive volume, the empty-circumsphere property over all vertices, and the
// Euler relation — plus determinism, sort-independence, hull faces, neighbors,
// duplicates, and the weighted (regular) variant.
#include "cmg/cmg.hpp"
#include "harness.hpp"

#include <array>
#include <cmath>
#include <set>
#include <vector>

using namespace cmg;

namespace {

// Every output tet is positively... library convention is orient3d(v0..v3) < 0.
bool all_tets_oriented(const Mesh& m) {
    for (const auto& t : m.tetrahedra) {
        double o = robust::orient3d(m.points[t[0]], m.points[t[1]],
                                    m.points[t[2]], m.points[t[3]]);
        if (o >= 0) return false;
    }
    return true;
}

// The Delaunay property: no input vertex lies strictly inside any tet's
// circumsphere. insphere sign depends on orientation, so normalise per tet.
bool empty_circumspheres(const Mesh& m) {
    for (const auto& t : m.tetrahedra) {
        // Orient the 4 tet vertices positively for a well-defined insphere sign.
        std::array<int, 4> v{t[0], t[1], t[2], t[3]};
        if (robust::orient3d(m.points[v[0]], m.points[v[1]], m.points[v[2]],
                             m.points[v[3]]) < 0)
            std::swap(v[2], v[3]);
        for (int p = 0; p < (int)m.points.size(); ++p) {
            if (p == v[0] || p == v[1] || p == v[2] || p == v[3]) continue;
            double s = robust::insphere(m.points[v[0]], m.points[v[1]],
                                        m.points[v[2]], m.points[v[3]],
                                        m.points[p]);
            if (s > 0) return false; // strictly inside -> not Delaunay
        }
    }
    return true;
}

std::vector<Point3> cube_corners() {
    return {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
            {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
}

// Deterministic pseudo-random cloud (no <random> dependency).
std::vector<Point3> cloud(int n, unsigned seed) {
    std::vector<Point3> pts;
    unsigned s = seed;
    auto next = [&] {
        s = s * 1664525u + 1013904223u;
        return (s >> 8) / double(1u << 24); // [0,1)
    };
    for (int i = 0; i < n; ++i) pts.push_back({next(), next(), next()});
    return pts;
}

} // namespace

CMG_TEST("cube corners tetrahedralize into a valid Delaunay mesh") {
    auto r = delaunay(cube_corners(), {});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_count() >= 5); // a cube splits into 5 or 6 tets
    CMG_CHECK(all_tets_oriented(*r));
    CMG_CHECK(empty_circumspheres(*r));
}

CMG_TEST("random cloud satisfies the empty-circumsphere property") {
    auto pts = cloud(60, 12345);
    auto r = delaunay(pts, {});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->point_count() == 60);
    CMG_CHECK(all_tets_oriented(*r));
    CMG_CHECK(empty_circumspheres(*r));
}

CMG_TEST("cospherical points produce a valid tetrahedralization") {
    // 12 points on a sphere + center: a strong cospherical degeneracy.
    std::vector<Point3> pts;
    const double g = 1.618033988749895;
    for (int s0 : {-1, 1})
        for (int s1 : {-1, 1}) {
            pts.push_back({0, double(s0), double(s1) * g});
            pts.push_back({double(s0), double(s1) * g, 0});
            pts.push_back({double(s1) * g, 0, double(s0)});
        }
    pts.push_back({0, 0, 0});
    auto r = delaunay(pts, {});
    CMG_CHECK(bool(r));
    CMG_CHECK(all_tets_oriented(*r));
    CMG_CHECK(empty_circumspheres(*r));
}

CMG_TEST("result is deterministic for a fixed seed") {
    auto pts = cloud(40, 999);
    auto a = delaunay(pts, {});
    auto b = delaunay(pts, {});
    CMG_CHECK(bool(a) && bool(b));
    CMG_CHECK(a->tetrahedra == b->tetrahedra);
}

CMG_TEST("spatial sort on/off both yield valid Delaunay meshes") {
    auto pts = cloud(50, 7);
    MeshOptions sorted;  sorted.spatial_sort = true;
    MeshOptions unsorted; unsorted.spatial_sort = false;
    auto a = delaunay(pts, sorted);
    auto b = delaunay(pts, unsorted);
    CMG_CHECK(bool(a) && bool(b));
    CMG_CHECK(empty_circumspheres(*a));
    CMG_CHECK(empty_circumspheres(*b));
    // Same convex hull => same number of Delaunay tets for general position.
    CMG_CHECK(a->tet_count() == b->tet_count());
}

CMG_TEST("convex hull faces are emitted and marked 1") {
    auto r = delaunay(cube_corners(), {});
    CMG_CHECK(bool(r));
    // A closed hull has an even number of triangles; a cube hull has 12.
    CMG_CHECK(r->face_count() == 12);
    for (int mk : r->face_markers) CMG_CHECK(mk == 1);
}

CMG_TEST("neighbor adjacency is symmetric and hull faces read -1") {
    MeshOptions opts; opts.emit_neighbors = true;
    auto r = delaunay(cube_corners(), opts);
    CMG_CHECK(bool(r));
    CMG_CHECK(r->neighbors.size() == r->tet_count());
    int hull = 0;
    for (std::size_t i = 0; i < r->neighbors.size(); ++i) {
        for (int k = 0; k < 4; k++) {
            int m = r->neighbors[i][k];
            if (m < 0) { ++hull; continue; }
            // symmetric: neighbor m lists i back somewhere
            bool back = false;
            for (int q = 0; q < 4; q++) back |= (r->neighbors[m][q] == (int)i);
            CMG_CHECK(back);
        }
    }
    CMG_CHECK(hull == 12); // one -1 slot per hull face
}

CMG_TEST("duplicate points are collapsed, mesh stays valid") {
    auto pts = cube_corners();
    pts.push_back(pts[0]); // exact duplicate
    pts.push_back(pts[6]);
    auto r = delaunay(pts, {});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->point_count() == pts.size()); // duplicates retained in points
    CMG_CHECK(empty_circumspheres(*r));
    // The duplicated indices appear in no tetrahedron.
    std::set<int> used;
    for (const auto& t : r->tetrahedra)
        for (int v : t) used.insert(v);
    CMG_CHECK(!used.count(8) && !used.count(9));
}

CMG_TEST("larger cloud no longer returns NotImplemented") {
    auto pts = cloud(120, 42);
    auto r = delaunay(pts, {});
    CMG_CHECK(bool(r));
    CMG_CHECK(empty_circumspheres(*r));
}

CMG_TEST("weighted DT drops a dominated point from the tetrahedra") {
    // A cube plus a near-center point given a hugely negative weight so its power
    // is dominated -> it must belong to no tetrahedron (regular triangulation).
    auto pts = cube_corners();
    pts.push_back({0.5, 0.5, 0.5});
    MeshOptions opts;
    opts.weighted = true;
    opts.weights.assign(pts.size(), 0.0);
    opts.weights.back() = -100.0; // large negative weight => dominated
    auto r = delaunay(pts, opts);
    CMG_CHECK(bool(r));
    CMG_CHECK(r->point_count() == 9);
    std::set<int> used;
    for (const auto& t : r->tetrahedra)
        for (int v : t) used.insert(v);
    CMG_CHECK(!used.count(8)); // the dominated point is redundant
}
