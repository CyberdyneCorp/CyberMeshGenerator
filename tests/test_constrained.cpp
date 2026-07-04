// CyberMeshGenerator — Phase 3 boundary-conforming tetrahedralization tests.
#include "cmg/cmg.hpp"
#include "cmg/io/io.hpp"
#include "harness.hpp"

#include <array>
#include <cmath>
#include <set>
#include <vector>

using namespace cmg;

namespace {

double tet_volume(const Point3& a, const Point3& b, const Point3& c,
                  const Point3& d) {
    double m[3][3] = {{b.x - a.x, c.x - a.x, d.x - a.x},
                      {b.y - a.y, c.y - a.y, d.y - a.y},
                      {b.z - a.z, c.z - a.z, d.z - a.z}};
    double det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                 m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                 m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    return std::fabs(det) / 6.0;
}

double mesh_volume(const Mesh& m) {
    double v = 0;
    for (const auto& t : m.tetrahedra)
        v += tet_volume(m.points[t[0]], m.points[t[1]], m.points[t[2]],
                        m.points[t[3]]);
    return v;
}

// Cube [0,1]^3 as a PLC of 12 triangles (2 per square face).
PLC cube_plc() {
    PLC p;
    p.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    const int q[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                         {2, 3, 7, 6}, {1, 2, 6, 5}, {0, 4, 7, 3}};
    for (auto& f : q) {
        Facet facet;
        facet.polygons.push_back({{f[0], f[1], f[2]}});
        facet.polygons.push_back({{f[0], f[2], f[3]}});
        facet.marker = 1;
        p.facets.push_back(facet);
    }
    return p;
}

} // namespace

CMG_TEST("single-tetrahedron PLC yields one tetrahedron") {
    PLC p;
    p.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const int tri[4][3] = {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}};
    for (auto& t : tri) {
        Facet f;
        f.polygons.push_back({{t[0], t[1], t[2]}});
        p.facets.push_back(f);
    }
    auto r = tetrahedralize(p, MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_count() == 1);
    CMG_CHECK(std::fabs(mesh_volume(*r) - 1.0 / 6.0) < 1e-9);
}

CMG_TEST("cube PLC tetrahedralizes with conserved volume") {
    auto r = tetrahedralize(cube_plc(), MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_count() >= 5);
    CMG_CHECK(std::fabs(mesh_volume(*r) - 1.0) < 1e-9); // convex => exact
}

CMG_TEST("cube boundary faces lie on the cube surface and border one tet") {
    auto r = tetrahedralize(cube_plc(), MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    // Every boundary face has all three vertices on a cube face plane (a coord
    // equal to 0 or 1 shared by all three).
    for (const auto& f : r->faces) {
        bool on_plane = false;
        for (int axis = 0; axis < 3 && !on_plane; ++axis)
            for (double plane : {0.0, 1.0}) {
                if (std::fabs(r->points[f[0]][axis] - plane) < 1e-9 &&
                    std::fabs(r->points[f[1]][axis] - plane) < 1e-9 &&
                    std::fabs(r->points[f[2]][axis] - plane) < 1e-9) {
                    on_plane = true;
                    break;
                }
            }
        CMG_CHECK(on_plane);
    }
    // A closed cube surface triangulates into 12 boundary triangles.
    CMG_CHECK(r->face_count() == 12);
}

CMG_TEST("carving removes exterior tetrahedra (hull larger than domain)") {
    // A flat "plus"/notched prism: the convex hull is a box but the domain is an
    // L-shaped prism, so some hull tetrahedra must be carved away.
    PLC p;
    // L-shape base (z=0 and z=1): an L polygon of 6 corners, extruded.
    std::vector<std::array<double, 2>> poly = {
        {0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}};
    for (double z : {0.0, 1.0})
        for (auto& xy : poly)
            p.points.push_back({static_cast<Real>(xy[0]),
                                static_cast<Real>(xy[1]), static_cast<Real>(z)});
    // 6 side quads + top/bottom L-caps (fan) as triangles.
    auto tri = [&](int a, int b, int c) {
        Facet f; f.polygons.push_back({{a, b, c}}); p.facets.push_back(f);
    };
    int n = 6;
    for (int i = 0; i < n; ++i) { // side walls
        int j = (i + 1) % n;
        tri(i, j, n + j); tri(i, n + j, n + i);
    }
    for (int i = 2; i < n; ++i) { tri(0, i - 1, i); tri(n, n + i, n + i - 1); }

    auto r = tetrahedralize(p, MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    double v = mesh_volume(*r);
    // L-area = 3 (2x2 box minus 1x1 notch), height 1 => domain volume 3.
    CMG_CHECK(std::fabs(v - 3.0) < 1e-6);
    // The convex hull (2x2x1 box) has volume 4 > 3, so carving happened.
    CMG_CHECK(v < 4.0 - 1e-6);
}

CMG_TEST("faceted PLC via cmg::io round-trips into a mesh (dogfood Phase 2)") {
    // Write the cube PLC to .poly (multi-polygon facets), read it back, mesh it.
    auto w = io::write_plc("/tmp/cmg_cube.poly", cube_plc());
    CMG_CHECK(bool(w));
    auto plc = io::read_plc("/tmp/cmg_cube.poly");
    CMG_CHECK(bool(plc));
    auto r = tetrahedralize(*plc, MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    CMG_CHECK(std::fabs(mesh_volume(*r) - 1.0) < 1e-9);
}

CMG_TEST("coplanar PLC returns InvalidInput") {
    PLC p;
    p.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    Facet f;
    f.polygons.push_back({{0, 1, 2, 3}});
    p.facets.push_back(f);
    auto r = tetrahedralize(p, MeshOptions{.plc = true});
    CMG_CHECK(!r);
    CMG_CHECK(r.error().code == MeshErrorCode::InvalidInput);
}
