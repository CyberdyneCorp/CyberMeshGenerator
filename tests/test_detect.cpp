// CyberMeshGenerator — tests for PLC self-intersection detection.
#include <array>

#include "cmg/cmg.hpp"
#include "cmg/detect/self_intersection.hpp"
#include "harness.hpp"

using cmg::Facet;
using cmg::PLC;
using cmg::Point3;
using cmg::Polygon;
using cmg::detect::FacetIntersection;
using cmg::detect::self_intersections;

namespace {

Facet make_facet(std::initializer_list<int> verts) {
    Facet f;
    Polygon poly;
    poly.vertices.assign(verts.begin(), verts.end());
    f.polygons.push_back(std::move(poly));
    return f;
}

} // namespace

// (a) Two triangles that genuinely pierce each other: one in the z=0 plane, one
// in the x=0 plane, arranged so each crosses the other's interior.
CMG_TEST("self_intersections: crossing triangles are reported") {
    PLC plc;
    // Facet 1 (z=0 plane): A,B,C spanning the origin.
    plc.points.push_back(Point3{-1, -1, 0}); // 0 A
    plc.points.push_back(Point3{1, -1, 0});  // 1 B
    plc.points.push_back(Point3{0, 1, 0});   // 2 C
    // Facet 2 (x=0 plane): D,E below, F above -> crosses z=0 inside facet 1.
    plc.points.push_back(Point3{0, -1, -1}); // 3 D
    plc.points.push_back(Point3{0, 1, -1});  // 4 E
    plc.points.push_back(Point3{0, 0, 1});   // 5 F

    plc.facets.push_back(make_facet({0, 1, 2}));
    plc.facets.push_back(make_facet({3, 4, 5}));

    const std::vector<FacetIntersection> hits = self_intersections(plc);
    CMG_CHECK(!hits.empty());

    bool found = false;
    for (const FacetIntersection& h : hits) {
        const bool pair01 = (h.facet_a == 0 && h.facet_b == 1) ||
                            (h.facet_a == 1 && h.facet_b == 0);
        if (pair01) found = true;
    }
    CMG_CHECK(found);
}

// (b) A well-formed unit cube: 6 quad facets, fan-triangulated into 2 triangles
// each. Faces meet only along shared edges/vertices -> no self-intersection.
CMG_TEST("self_intersections: valid cube is clean") {
    PLC plc;
    plc.points.push_back(Point3{0, 0, 0}); // 0
    plc.points.push_back(Point3{1, 0, 0}); // 1
    plc.points.push_back(Point3{1, 1, 0}); // 2
    plc.points.push_back(Point3{0, 1, 0}); // 3
    plc.points.push_back(Point3{0, 0, 1}); // 4
    plc.points.push_back(Point3{1, 0, 1}); // 5
    plc.points.push_back(Point3{1, 1, 1}); // 6
    plc.points.push_back(Point3{0, 1, 1}); // 7

    plc.facets.push_back(make_facet({0, 1, 2, 3})); // bottom z=0
    plc.facets.push_back(make_facet({4, 5, 6, 7})); // top    z=1
    plc.facets.push_back(make_facet({0, 1, 5, 4})); // front  y=0
    plc.facets.push_back(make_facet({2, 3, 7, 6})); // back   y=1
    plc.facets.push_back(make_facet({1, 2, 6, 5})); // right  x=1
    plc.facets.push_back(make_facet({0, 3, 7, 4})); // left   x=0

    const std::vector<FacetIntersection> hits = self_intersections(plc);
    CMG_CHECK(hits.empty());
}

// (c) A single tetrahedron: 4 triangular facets sharing only edges/vertices.
CMG_TEST("self_intersections: single tetrahedron is clean") {
    PLC plc;
    plc.points.push_back(Point3{0, 0, 0}); // 0
    plc.points.push_back(Point3{1, 0, 0}); // 1
    plc.points.push_back(Point3{0, 1, 0}); // 2
    plc.points.push_back(Point3{0, 0, 1}); // 3

    plc.facets.push_back(make_facet({0, 1, 2}));
    plc.facets.push_back(make_facet({0, 1, 3}));
    plc.facets.push_back(make_facet({0, 2, 3}));
    plc.facets.push_back(make_facet({1, 2, 3}));

    const std::vector<FacetIntersection> hits = self_intersections(plc);
    CMG_CHECK(hits.empty());
}
