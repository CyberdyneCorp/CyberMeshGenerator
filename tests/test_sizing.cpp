// CyberMeshGenerator — Phase 5 adaptive mesh sizing tests.
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

Point3 centroid(const Mesh& m, const Tetrahedron& t) {
    return {(m.points[t[0]].x + m.points[t[1]].x + m.points[t[2]].x + m.points[t[3]].x) / 4,
            (m.points[t[0]].y + m.points[t[1]].y + m.points[t[2]].y + m.points[t[3]].y) / 4,
            (m.points[t[0]].z + m.points[t[1]].z + m.points[t[2]].z + m.points[t[3]].z) / 4};
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

CMG_TEST("graded sizing makes the small-size region finer") {
    MeshOptions o;
    o.plc = true;
    o.sizing = [](const Point3& p) { return p.x < 0.5 ? 0.25 : 0.7; };
    auto r = tetrahedralize(cube_plc(), o);
    CMG_CHECK(bool(r));

    int nf = 0, nc = 0;
    double vf = 0, vc = 0;
    for (const auto& t : r->tetrahedra) {
        double v = tet_volume(r->points[t[0]], r->points[t[1]], r->points[t[2]],
                              r->points[t[3]]);
        if (centroid(*r, t).x < 0.5) { ++nf; vf += v; }
        else { ++nc; vc += v; }
    }
    CMG_CHECK(nf > nc);                    // more tets in the fine half
    CMG_CHECK((vf / nf) < 0.5 * (vc / nc)); // and markedly smaller ones
}

CMG_TEST("size zero leaves a region unrefined") {
    // Refine only near the x=0 face; size 0 elsewhere.
    MeshOptions o;
    o.plc = true;
    o.sizing = [](const Point3& p) { return p.x < 0.25 ? 0.2 : 0.0; };
    auto r = tetrahedralize(cube_plc(), o);
    CMG_CHECK(bool(r));
    // Tets far from x=0 keep large volume (unconstrained region not refined).
    double max_far = 0;
    for (const auto& t : r->tetrahedra)
        if (centroid(*r, t).x > 0.6)
            max_far = std::max(max_far, tet_volume(r->points[t[0]], r->points[t[1]],
                                                   r->points[t[2]], r->points[t[3]]));
    CMG_CHECK(max_far > 0.05); // coarse tets survive where size is 0
}

CMG_TEST("background-mesh sizing interpolates and is exact at nodes") {
    // Background = the 6-tet cube DT; node size grows with x.
    auto bg = delaunay(cube_pts(), {});
    CMG_CHECK(bool(bg));
    std::vector<double> sizes;
    for (const auto& p : bg->points) sizes.push_back(0.2 + 0.5 * p.x);
    auto f = sizing::from_background(*bg, sizes);
    // Exact at a node.
    CMG_CHECK(std::fabs(f(bg->points[1]) - (0.2 + 0.5 * bg->points[1].x)) < 1e-6);
    // Interpolated at the centre ~ mid-range.
    double mid = f({0.5, 0.5, 0.5});
    CMG_CHECK(mid > 0.2 && mid < 0.7);
    CMG_CHECK(std::fabs(mid - 0.45) < 0.05);
}

CMG_TEST("metric scaling shrinks the interpolated sizes") {
    auto bg = delaunay(cube_pts(), {});
    std::vector<double> sizes(bg->points.size(), 0.4);
    auto full = sizing::from_background(*bg, sizes, 1.0);
    auto half = sizing::from_background(*bg, sizes, 0.5);
    Point3 q{0.5, 0.5, 0.5};
    CMG_CHECK(std::fabs(half(q) - 0.5 * full(q)) < 1e-9);
    CMG_CHECK(std::fabs(full(q) - 0.4) < 1e-6);
}

CMG_TEST("background sizing drives finer refinement in the small-size region") {
    auto bg = delaunay(cube_pts(), {});
    std::vector<double> sizes;
    for (const auto& p : bg->points) sizes.push_back(p.z < 0.5 ? 0.25 : 0.9);
    MeshOptions o;
    o.plc = true;
    o.sizing = sizing::from_background(*bg, sizes);
    auto r = tetrahedralize(cube_plc(), o);
    CMG_CHECK(bool(r));
    int low = 0, high = 0;
    for (const auto& t : r->tetrahedra)
        (centroid(*r, t).z < 0.5 ? low : high)++;
    CMG_CHECK(low > high); // the small-size (low-z) region is finer
}

CMG_TEST("sizing combined with max_volume takes the tighter target") {
    MeshOptions o;
    o.plc = true;
    o.max_volume = 0.02;                             // global-ish
    o.sizing = [](const Point3& p) { return p.x < 0.3 ? 0.2 : 100.0; }; // tiny in a strip
    auto r = tetrahedralize(cube_plc(), o);
    CMG_CHECK(bool(r));
    // Everywhere obeys max_volume; the x<0.3 strip is finer still.
    double max_all = 0, max_strip = 0;
    for (const auto& t : r->tetrahedra) {
        double v = tet_volume(r->points[t[0]], r->points[t[1]], r->points[t[2]],
                              r->points[t[3]]);
        max_all = std::max(max_all, v);
        if (centroid(*r, t).x < 0.3) max_strip = std::max(max_strip, v);
    }
    CMG_CHECK(max_all <= 0.02 + 1e-6);
    CMG_CHECK(max_strip < 0.02); // strip strictly finer than the global cap
}

CMG_TEST("no sizing and no volume leaves the mesh unrefined") {
    auto r = tetrahedralize(cube_plc(), MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->point_count() == 8);
}
