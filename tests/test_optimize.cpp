// CyberMeshGenerator — Laplacian smoothing + dihedral quality tests.
#include "cmg/cmg.hpp"
#include "cmg/optimize/smooth.hpp"
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

// Cube [0,1]^3 as a PLC of 8 points and 6 facets (2 triangles each).
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

CMG_TEST("laplacian smoothing preserves boundary, orientation, and volume") {
    auto r = tetrahedralize(cube_plc(),
                            MeshOptions{.plc = true, .max_volume = 0.05});
    CMG_CHECK(r.has_value());
    const Mesh& before = *r;
    CMG_CHECK(before.tet_count() > 0);

    // Vertices used on the boundary (appear in some face).
    std::set<int> boundary;
    for (const auto& f : before.faces)
        for (int k = 0; k < 3; ++k) boundary.insert(f[k]);
    CMG_CHECK(!boundary.empty());

    const double before_min = optimize::min_dihedral_angle(before);
    const double before_vol = mesh_volume(before);

    Mesh after = optimize::laplacian_smooth(before, {.iterations = 5,
                                                     .relaxation = 1.0});

    // (a) Boundary vertices are unchanged.
    for (int b : boundary) CMG_CHECK(after.points[b] == before.points[b]);

    // (b) No tetrahedron inverted (stored orientation stays negative).
    for (const auto& t : after.tetrahedra)
        CMG_CHECK(robust::orient3d(after.points[t[0]], after.points[t[1]],
                                   after.points[t[2]],
                                   after.points[t[3]]) < 0.0);

    // (c) Total volume conserved.
    CMG_CHECK(std::fabs(mesh_volume(after) - before_vol) < 1e-6);

    // (d) Minimum dihedral angle does not get worse.
    const double after_min = optimize::min_dihedral_angle(after);
    CMG_CHECK(after_min >= before_min - 1e-9);
}

CMG_TEST("min_dihedral_angle of a regular tetrahedron is ~70.53 deg") {
    // A regular tetrahedron stored with orient3d < 0.
    Mesh m;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0.5, std::sqrt(3.0) / 2.0, 0},
                {0.5, std::sqrt(3.0) / 6.0, std::sqrt(2.0 / 3.0)}};
    // Order the 4 vertices so orient3d(v0,v1,v2,v3) < 0.
    Tetrahedron t = {0, 1, 2, 3};
    if (robust::orient3d(m.points[t[0]], m.points[t[1]], m.points[t[2]],
                         m.points[t[3]]) > 0)
        std::swap(t[2], t[3]);
    m.tetrahedra.push_back(t);

    const double d = optimize::min_dihedral_angle(m);
    CMG_CHECK(std::fabs(d - 70.5288) < 1e-2);
}
