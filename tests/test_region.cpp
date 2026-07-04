// CyberMeshGenerator — Phase 6 region attributes and hole tests.
#include "cmg/cmg.hpp"
#include "harness.hpp"

#include <set>
#include <vector>

using namespace cmg;

namespace {

// Append a unit cube translated by (ox,0,0) to the PLC (8 points, 6 facets of 2
// triangles each). Returns the base point index of the added cube.
int add_cube(PLC& p, double ox) {
    int base = static_cast<int>(p.points.size());
    const double c[8][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                            {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    for (auto& v : c)
        p.points.push_back({static_cast<Real>(v[0] + ox),
                            static_cast<Real>(v[1]), static_cast<Real>(v[2])});
    const int q[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                         {2, 3, 7, 6}, {1, 2, 6, 5}, {0, 4, 7, 3}};
    for (auto& f : q) {
        Facet facet;
        facet.polygons.push_back({{base + f[0], base + f[1], base + f[2]}});
        facet.polygons.push_back({{base + f[0], base + f[2], base + f[3]}});
        p.facets.push_back(facet);
    }
    return base;
}

PLC two_cubes() {
    PLC p;
    add_cube(p, 0.0);
    add_cube(p, 3.0); // gap of 2 so the convex-hull bridge is carved away
    return p;
}

} // namespace

CMG_TEST("single region seed marks every tetrahedron") {
    PLC p;
    add_cube(p, 0.0);
    p.regions.push_back({{0.5, 0.5, 0.5}, 5.0, -1.0});
    auto r = tetrahedralize(p, MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_markers.size() == r->tet_count());
    for (int m : r->tet_markers) CMG_CHECK(m == 5);
}

CMG_TEST("two separated solids receive their own attributes") {
    PLC p = two_cubes();
    p.regions.push_back({{0.5, 0.5, 0.5}, 1.0, -1.0}); // cube A
    p.regions.push_back({{3.5, 0.5, 0.5}, 2.0, -1.0}); // cube B
    auto r = tetrahedralize(p, MeshOptions{.plc = true});
    CMG_CHECK(bool(r));

    std::set<int> attrs;
    for (std::size_t i = 0; i < r->tetrahedra.size(); ++i) {
        // centroid x tells which cube a tet is in
        double cx = 0;
        for (int k = 0; k < 4; k++) cx += r->points[r->tetrahedra[i][k]].x / 4;
        int want = (cx < 1.5) ? 1 : 2;
        CMG_CHECK(r->tet_markers[i] == want);
        attrs.insert(r->tet_markers[i]);
    }
    CMG_CHECK(attrs.count(1) && attrs.count(2)); // both regions present
}

CMG_TEST("hole seed removes its solid and keeps the other") {
    PLC p = two_cubes();
    p.holes.push_back({0.5, 0.5, 0.5}); // remove cube A
    auto r = tetrahedralize(p, MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_count() > 0);
    // Every surviving tet is in cube B (x > 2).
    for (const auto& t : r->tetrahedra) {
        double cx = 0;
        for (int k = 0; k < 4; k++) cx += r->points[t[k]].x / 4;
        CMG_CHECK(cx > 2.0);
    }
}

CMG_TEST("auto-labeling gives each component a distinct nonzero attribute") {
    PLC p = two_cubes();
    MeshOptions o;
    o.plc = true;
    o.label_regions = true;
    auto r = tetrahedralize(p, o);
    CMG_CHECK(bool(r));
    std::set<int> labels(r->tet_markers.begin(), r->tet_markers.end());
    CMG_CHECK(labels.size() == 2);   // two distinct labels
    CMG_CHECK(!labels.count(0));     // all nonzero
}

CMG_TEST("no regions or holes leaves attributes at zero") {
    PLC p;
    add_cube(p, 0.0);
    auto r = tetrahedralize(p, MeshOptions{.plc = true});
    CMG_CHECK(bool(r));
    for (int m : r->tet_markers) CMG_CHECK(m == 0);
}
