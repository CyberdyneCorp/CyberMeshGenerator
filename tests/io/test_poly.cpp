// CyberMeshGenerator — round-trip tests for the .poly / .smesh PLC formats.
#include <fstream>

#include "cmg/core/plc.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

namespace {

/// A small tetrahedron-shaped PLC with four points, four triangular facets,
/// one volumetric hole, and one region.
PLC make_plc() {
    PLC plc;
    plc.index_base = IndexBase::Zero;
    plc.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    plc.facets.push_back({{Polygon{{0, 1, 2}}}, {}, 11});
    plc.facets.push_back({{Polygon{{0, 1, 3}}}, {}, 12});
    plc.facets.push_back({{Polygon{{0, 2, 3}}}, {}, 13});
    plc.facets.push_back({{Polygon{{1, 2, 3}}}, {}, 14});
    plc.holes.push_back({0.25, 0.25, 0.25});
    plc.regions.push_back({{0.1, 0.1, 0.1}, 7.0, 0.5});
    return plc;
}

void check_roundtrip(const PLC& a, const PLC& b) {
    CMG_CHECK(a.points.size() == b.points.size());
    for (std::size_t i = 0; i < a.points.size(); ++i)
        CMG_CHECK(a.points[i] == b.points[i]);
    CMG_CHECK(a.facets.size() == b.facets.size());
    for (std::size_t i = 0; i < a.facets.size(); ++i) {
        CMG_CHECK(a.facets[i].marker == b.facets[i].marker);
        CMG_CHECK(a.facets[i].polygons.size() == b.facets[i].polygons.size());
        CMG_CHECK(a.facets[i].polygons[0].vertices ==
                  b.facets[i].polygons[0].vertices);
    }
    CMG_CHECK(a.holes.size() == b.holes.size());
    for (std::size_t i = 0; i < a.holes.size(); ++i)
        CMG_CHECK(a.holes[i] == b.holes[i]);
    CMG_CHECK(a.regions.size() == b.regions.size());
    for (std::size_t i = 0; i < a.regions.size(); ++i) {
        CMG_CHECK(a.regions[i].seed == b.regions[i].seed);
        CMG_CHECK(a.regions[i].attribute == b.regions[i].attribute);
        CMG_CHECK(a.regions[i].max_volume == b.regions[i].max_volume);
    }
}

} // namespace

CMG_TEST("poly round-trip preserves points, facets, holes, regions") {
    const std::string path = "/tmp/cmg_io_poly.poly";
    PLC plc = make_plc();
    auto w = io::write_poly(path, plc);
    CMG_CHECK(static_cast<bool>(w));
    auto r = io::read_poly(path);
    CMG_CHECK(static_cast<bool>(r));
    check_roundtrip(plc, r.value());
}

CMG_TEST("smesh round-trip preserves single-polygon facets and markers") {
    const std::string path = "/tmp/cmg_io_smesh.smesh";
    PLC plc = make_plc();
    auto w = io::write_smesh(path, plc);
    CMG_CHECK(static_cast<bool>(w));
    auto r = io::read_smesh(path);
    CMG_CHECK(static_cast<bool>(r));
    check_roundtrip(plc, r.value());
}

CMG_TEST("poly with index base one round-trips") {
    const std::string path = "/tmp/cmg_io_poly_base1.poly";
    PLC plc = make_plc();
    plc.index_base = IndexBase::One;
    auto w = io::write_poly(path, plc);
    CMG_CHECK(static_cast<bool>(w));
    auto r = io::read_poly(path);
    CMG_CHECK(static_cast<bool>(r));
    CMG_CHECK(r.value().index_base == IndexBase::One);
    // Internal vertex indices stay 0-based regardless of the file base.
    check_roundtrip(plc, r.value());
}

CMG_TEST("truncated poly facet section returns an error") {
    const std::string path = "/tmp/cmg_io_poly_bad.poly";
    {
        std::ofstream out(path);
        // Four points, then a facet header promising 2 facets but no facet data.
        out << "4 3 0 0\n"
            << "0 0 0 0\n1 1 0 0\n2 0 1 0\n3 0 0 1\n"
            << "2 1\n";
    }
    auto r = io::read_poly(path);
    CMG_CHECK(!r);
}

CMG_TEST("malformed smesh corner count returns an error") {
    const std::string path = "/tmp/cmg_io_smesh_bad.smesh";
    {
        std::ofstream out(path);
        out << "4 3 0 0\n"
            << "0 0 0 0\n1 1 0 0\n2 0 1 0\n3 0 0 1\n"
            << "1 1\n"
            << "xyz 0 1 2\n"; // non-integer corner count
    }
    auto r = io::read_smesh(path);
    CMG_CHECK(!r);
}
