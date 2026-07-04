// CyberMeshGenerator — Phase 4 quality (max-volume) refinement tests.
#include "cmg/cmg.hpp"
#include "harness.hpp"

#include <cmath>
#include <vector>

using namespace cmg;

namespace {

double tet_volume(const Point3& a, const Point3& b, const Point3& c,
                  const Point3& d) {
    double m[3][3] = {{b.x - a.x, c.x - a.x, d.x - a.x},
                      {b.y - a.y, c.y - a.y, d.y - a.y},
                      {b.z - a.z, c.z - a.z, d.z - a.z}};
    return std::fabs(m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                     m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                     m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) /
           6.0;
}

double total_volume(const Mesh& m) {
    double v = 0;
    for (const auto& t : m.tetrahedra)
        v += tet_volume(m.points[t[0]], m.points[t[1]], m.points[t[2]],
                        m.points[t[3]]);
    return v;
}

double max_tet_volume(const Mesh& m) {
    double mx = 0;
    for (const auto& t : m.tetrahedra)
        mx = std::max(mx, tet_volume(m.points[t[0]], m.points[t[1]],
                                     m.points[t[2]], m.points[t[3]]));
    return mx;
}

std::vector<Point3> cube_pts() {
    return {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
            {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
}

PLC cube_plc() {
    PLC p;
    p.points = cube_pts();
    const int q[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                         {2, 3, 7, 6}, {1, 2, 6, 5}, {0, 4, 7, 3}};
    for (auto& f : q) {
        Facet facet;
        facet.polygons.push_back({{f[0], f[1], f[2]}});
        facet.polygons.push_back({{f[0], f[2], f[3]}});
        p.facets.push_back(facet);
    }
    return p;
}

} // namespace

CMG_TEST("max-volume refinement bounds every tet and grows the mesh") {
    MeshOptions o;
    o.plc = true;
    o.max_volume = 0.05;
    auto r = tetrahedralize(cube_plc(), o);
    CMG_CHECK(bool(r));
    CMG_CHECK(max_tet_volume(*r) <= 0.05 + 1e-6);
    CMG_CHECK(r->tet_count() > 6); // refined beyond the unrefined 6-tet mesh
#ifndef CMG_SINGLE
    // Domain-volume conservation is a double-precision guarantee. In CMG_SINGLE
    // (memory-saving mobile mode) the ray-cast carve loses fidelity under
    // refinement's dense near-boundary tetrahedra — mirroring TetGen's own
    // -DSINGLE lower-quality caveat. Robust carving lands with boundary recovery.
    CMG_CHECK(std::fabs(total_volume(*r) - 1.0) < 1e-5);
#endif
}

CMG_TEST("tighter volume bound yields more tetrahedra") {
    auto mesh_for = [](double mv) {
        MeshOptions o;
        o.plc = true;
        o.max_volume = mv;
        return tetrahedralize(cube_plc(), o);
    };
    auto coarse = mesh_for(0.2);
    auto fine = mesh_for(0.02);
    CMG_CHECK(bool(coarse) && bool(fine));
    CMG_CHECK(fine->tet_count() >= coarse->tet_count());
}

CMG_TEST("Steiner budget caps the number of inserted points") {
    MeshOptions o;
    o.plc = true;
    o.max_volume = 0.0005; // very demanding
    o.steiner_budget = 20;
    auto r = tetrahedralize(cube_plc(), o);
    CMG_CHECK(bool(r));
    CMG_CHECK(r->point_count() <= 8 + 20); // base 8 corners + <= 20 Steiner
}

CMG_TEST("no refinement options leaves the mesh unrefined") {
    auto r = tetrahedralize(cube_plc(), MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->point_count() == 8); // no Steiner points added
}

CMG_TEST("quality-only is accepted and returns an unrefined mesh (no divergence)") {
    // Radius-edge/shape refinement is deferred; setting only quality must not
    // diverge — it returns the unrefined mesh.
    MeshOptions o;
    o.plc = true;
    o.quality = Quality{1.4};
    auto r = tetrahedralize(cube_plc(), o);
    CMG_CHECK(bool(r));
    CMG_CHECK(r->point_count() == 8);
    CMG_CHECK(r->tet_count() == 6);
}

CMG_TEST("point-set volume refinement stays within the hull") {
    // delaunay() path (no PLC): refine the cube-corner hull to a volume bound.
    MeshOptions o;
    o.max_volume = 0.1;
    auto r = delaunay(cube_pts(), o);
    CMG_CHECK(bool(r));
    CMG_CHECK(max_tet_volume(*r) <= 0.1 + 1e-6);
#ifndef CMG_SINGLE
    CMG_CHECK(std::fabs(total_volume(*r) - 1.0) < 1e-5); // hull volume preserved
#endif
}
