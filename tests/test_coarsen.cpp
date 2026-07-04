// CyberMeshGenerator — mesh coarsening tests.
#include "cmg/cmg.hpp"
#include "cmg/coarsen/coarsen.hpp"
#include "harness.hpp"

#include <vector>

using namespace cmg;

namespace {

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
        p.facets.push_back(facet);
    }
    return p;
}

Mesh refined_cube() {
    MeshOptions o;
    o.plc = true;
    o.max_volume = 0.02; // dense enough to introduce interior vertices
    auto r = tetrahedralize(cube_plc(), o);
    cmgtest::check(bool(r), "refined cube tetrahedralize");
    return *r;
}

} // namespace

CMG_TEST("coarsen removes interior vertices and shrinks the mesh") {
    Mesh m = refined_cube();
    CMG_CHECK(m.point_count() > 8); // has interior (Steiner) vertices

    coarsen::CoarsenOptions co;
    co.keep_fraction = 0.3;
    co.seed = 1;
    auto c = coarsen::coarsen(m, co);
    CMG_CHECK(bool(c));
    CMG_CHECK(c->point_count() < m.point_count());
    CMG_CHECK(c->tet_count() < m.tet_count());
}

CMG_TEST("coarsen is deterministic for a fixed seed") {
    Mesh m = refined_cube();

    coarsen::CoarsenOptions co;
    co.keep_fraction = 0.3;
    co.seed = 1;
    auto a = coarsen::coarsen(m, co);
    auto b = coarsen::coarsen(m, co);
    CMG_CHECK(bool(a));
    CMG_CHECK(bool(b));
    CMG_CHECK(a->tetrahedra == b->tetrahedra);
}
