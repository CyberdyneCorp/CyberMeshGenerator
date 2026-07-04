// CyberMeshGenerator — STL round-trip tests.
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "cmg/core/mesh.hpp"
#include "cmg/core/plc.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

namespace {

// A unit tetrahedron surface (4 triangles) sharing 4 corner vertices.
PLC make_tetra_plc() {
    PLC plc;
    Point3 a{0, 0, 0}, b{1, 0, 0}, c{0, 1, 0}, d{0, 0, 1};
    plc.points = {a, b, c, d};
    auto tri = [](int i, int j, int k) {
        Facet f;
        Polygon p;
        p.vertices = {i, j, k};
        f.polygons.push_back(p);
        return f;
    };
    plc.facets = {tri(0, 1, 2), tri(0, 1, 3), tri(0, 2, 3), tri(1, 2, 3)};
    return plc;
}

} // namespace

CMG_TEST("stl: ascii round-trip merges coincident vertices") {
    const char* path = "/tmp/cmg_io_stl.stl";
    PLC in = make_tetra_plc();
    auto w = cmg::io::write_stl(path, in);
    CMG_CHECK(static_cast<bool>(w));

    auto r = cmg::io::read_stl(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& out = r.value();

    // 4 triangles preserved.
    CMG_CHECK(out.facets.size() == 4);
    // Merged back down to the 4 unique corners.
    CMG_CHECK(out.points.size() == 4);
    // Every facet is a triangle with in-range indices.
    for (const auto& f : out.facets) {
        CMG_CHECK(f.polygons.size() == 1);
        CMG_CHECK(f.polygons[0].vertices.size() == 3);
        CMG_CHECK(f.marker == 0);
        for (int v : f.polygons[0].vertices)
            CMG_CHECK(v >= 0 && v < static_cast<int>(out.points.size()));
    }
}

CMG_TEST("stl: binary file is read back correctly") {
    const char* path = "/tmp/cmg_io_stl_bin.stl";
    // One triangle, binary layout: 80 header + u32 count + 50 bytes.
    std::uint32_t count = 1;
    std::vector<char> buf(84 + 50, 0);
    std::memcpy(buf.data() + 80, &count, 4);
    float coords[12] = {0, 0, 0,   // normal
                        0, 0, 0,   // v0
                        2, 0, 0,   // v1
                        0, 3, 0};  // v2
    std::memcpy(buf.data() + 84, coords, sizeof(coords));
    {
        std::ofstream o(path, std::ios::binary);
        o.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    }
    auto r = cmg::io::read_stl(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& out = r.value();
    CMG_CHECK(out.facets.size() == 1);
    CMG_CHECK(out.points.size() == 3);
}

CMG_TEST("stl: mesh writer emits one facet per face") {
    const char* path = "/tmp/cmg_io_stl_mesh.stl";
    Mesh m;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    m.faces = {Triangle{0, 1, 2}};
    auto w = cmg::io::write_stl_mesh(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = cmg::io::read_stl(path);
    CMG_CHECK(static_cast<bool>(r));
    CMG_CHECK(r.value().facets.size() == 1);
    CMG_CHECK(r.value().points.size() == 3);
}

CMG_TEST("stl: truncated ascii vertex is an error") {
    const char* path = "/tmp/cmg_io_stl_bad.stl";
    {
        std::ofstream o(path);
        o << "solid bad\n"
             "  facet normal 0 0 1\n"
             "    outer loop\n"
             "      vertex 0 0\n"   // missing z
             "    endloop\n"
             "  endfacet\n"
             "endsolid bad\n";
    }
    auto r = cmg::io::read_stl(path);
    CMG_CHECK(!r);
}
