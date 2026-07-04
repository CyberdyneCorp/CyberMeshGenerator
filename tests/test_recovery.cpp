// CyberMeshGenerator — segment (feature-edge) recovery tests.
#include "cmg/cmg.hpp"
#include "cmg/recover/segments.hpp"
#include "harness.hpp"

#include <array>
#include <cmath>
#include <set>
#include <vector>

using namespace cmg;

namespace {

std::array<int, 2> ord(int a, int b) {
    return a < b ? std::array<int, 2>{a, b} : std::array<int, 2>{b, a};
}

std::set<std::array<int, 2>> mesh_edges(const Mesh& m) {
    std::set<std::array<int, 2>> e;
    for (const auto& t : m.tetrahedra)
        for (int i = 0; i < 4; ++i)
            for (int j = i + 1; j < 4; ++j) e.insert(ord(t[i], t[j]));
    return e;
}

// A configuration where the z-axis segment (0,1) is provably NOT a Delaunay edge:
// two points on the axis, three around the origin in the z=0 plane.
std::vector<Point3> axis_case() {
    return {{0, 0, 1}, {0, 0, -1}, {0.5f, 0, 0},
            {-0.25f, 0.433f, 0}, {-0.25f, -0.433f, 0}};
}

double tet_volume(const Point3& a, const Point3& b, const Point3& c,
                  const Point3& d) {
    double m[3][3] = {{b.x - a.x, c.x - a.x, d.x - a.x},
                      {b.y - a.y, c.y - a.y, d.y - a.y},
                      {b.z - a.z, c.z - a.z, d.z - a.z}};
    return std::fabs(m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                     m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                     m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) / 6.0;
}

double total_volume(const Mesh& m) {
    double v = 0;
    for (const auto& t : m.tetrahedra)
        v += tet_volume(m.points[t[0]], m.points[t[1]], m.points[t[2]], m.points[t[3]]);
    return v;
}

// Cube PLC with quad facets (edges = the 12 cube edges only, no diagonals).
PLC cube_quad() {
    PLC p;
    p.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    const int q[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                         {2, 3, 7, 6}, {1, 2, 6, 5}, {0, 4, 7, 3}};
    for (auto& f : q) {
        Facet facet;
        facet.polygons.push_back({{f[0], f[1], f[2], f[3]}}); // single quad
        p.facets.push_back(facet);
    }
    return p;
}

} // namespace

CMG_TEST("a non-Delaunay segment is recovered as an edge chain") {
    auto pts = axis_case();
    // Confirm the segment is missing to begin with.
    auto m0 = delaunay(pts, {});
    CMG_CHECK(!mesh_edges(*m0).count(ord(0, 1)));

    std::vector<std::array<int, 2>> segs{{0, 1}};
    auto r = recover::recover_segments(pts, segs, 1000);
    CMG_CHECK(r.complete);
    CMG_CHECK(r.steiner_added == 1);         // one midpoint suffices
    CMG_CHECK(r.points.size() == pts.size() + 1);

    // The augmented DT covers (0,1) by the chain 0-5-1 (5 = the inserted midpoint).
    auto m1 = delaunay(r.points, {});
    auto e = mesh_edges(*m1);
    CMG_CHECK(e.count(ord(0, 5)) && e.count(ord(5, 1)));
}

CMG_TEST("already-Delaunay segments get no Steiner points") {
    PLC cube = cube_quad();
    auto segs = recover::facet_segments(cube); // 12 cube edges
    CMG_CHECK(segs.size() == 12);
    auto r = recover::recover_segments(cube.points, segs, 1000);
    CMG_CHECK(r.complete);
    CMG_CHECK(r.steiner_added == 0); // cube edges are all Delaunay
}

CMG_TEST("budget bounds a recovery and reports incomplete") {
    auto pts = axis_case();
    std::vector<std::array<int, 2>> segs{{0, 1}};
    auto r = recover::recover_segments(pts, segs, 0); // no budget
    CMG_CHECK(!r.complete);
    CMG_CHECK(r.steiner_added == 0);
}

CMG_TEST("preserve_edges keeps every quad-cube edge as a mesh edge") {
    PLC cube = cube_quad();
    MeshOptions o;
    o.plc = true;
    o.preserve_edges = true;
    auto r = tetrahedralize(cube, o);
    CMG_CHECK(bool(r));
    CMG_CHECK(std::fabs(total_volume(*r) - 1.0) < 1e-6);
    auto e = mesh_edges(*r);
    for (const auto& s : recover::facet_segments(cube))
        CMG_CHECK(e.count(ord(s[0], s[1]))); // each cube edge present directly
}

CMG_TEST("preserve_edges is opt-in (off adds no recovery points)") {
    PLC cube = cube_quad();
    auto off = tetrahedralize(cube, MeshOptions{.plc = true});
    CMG_CHECK(bool(off));
    CMG_CHECK(off->point_count() == 8); // no Steiner points from recovery
}
