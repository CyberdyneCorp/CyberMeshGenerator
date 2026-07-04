// Round-trip and error-path tests for the Geomview OFF reader/writer.
#include "harness.hpp"

#include <fstream>
#include <string>

#include "cmg/core/mesh.hpp"
#include "cmg/core/plc.hpp"
#include "cmg/io/formats.hpp"

using namespace cmg;

namespace {

PLC make_tetra_plc() {
    PLC plc;
    plc.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    // Four triangular facets of a tetrahedron.
    const int tris[4][3] = {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}};
    for (const auto& t : tris) {
        Facet f;
        Polygon p;
        p.vertices = {t[0], t[1], t[2]};
        f.polygons.push_back(std::move(p));
        plc.facets.push_back(std::move(f));
    }
    return plc;
}

} // namespace

CMG_TEST("off: PLC write/read round-trip preserves geometry and connectivity") {
    const std::string path = "/tmp/cmg_io_off_plc.off";
    PLC in = make_tetra_plc();

    auto w = io::write_off(path, in);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_off(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& out = r.value();

    CMG_CHECK(out.points.size() == in.points.size());
    for (std::size_t i = 0; i < in.points.size(); ++i) {
        CMG_CHECK(out.points[i].x == in.points[i].x);
        CMG_CHECK(out.points[i].y == in.points[i].y);
        CMG_CHECK(out.points[i].z == in.points[i].z);
    }
    CMG_CHECK(out.facets.size() == in.facets.size());
    for (std::size_t i = 0; i < in.facets.size(); ++i) {
        const auto& a = in.facets[i].polygons.front().vertices;
        const auto& b = out.facets[i].polygons.front().vertices;
        CMG_CHECK(a == b);
    }
    CMG_CHECK(out.index_base == IndexBase::Zero);
}

CMG_TEST("off: 1-based index base is detected and normalized to 0-based") {
    const std::string path = "/tmp/cmg_io_off_onebased.off";
    {
        std::ofstream f(path);
        f << "OFF\n3 1 0\n"
          << "0 0 0\n1 0 0\n0 1 0\n"
          << "3 1 2 3\n"; // 1-based indices referring to the three vertices
    }
    auto r = io::read_off(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& plc = r.value();
    CMG_CHECK(plc.index_base == IndexBase::One);
    const auto& v = plc.facets.at(0).polygons.front().vertices;
    CMG_CHECK(v.size() == 3);
    CMG_CHECK(v[0] == 0 && v[1] == 1 && v[2] == 2);
}

CMG_TEST("off: mesh triangle faces round-trip through write_off_mesh") {
    const std::string path = "/tmp/cmg_io_off_mesh.off";
    Mesh m;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    m.faces = {{0, 1, 2}};

    auto w = io::write_off_mesh(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_off(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& plc = r.value();
    CMG_CHECK(plc.points.size() == 3);
    CMG_CHECK(plc.facets.size() == 1);
    const auto& v = plc.facets.front().polygons.front().vertices;
    CMG_CHECK(v.size() == 3);
    CMG_CHECK(v[0] == 0 && v[1] == 1 && v[2] == 2);
}

CMG_TEST("off: truncated vertex list returns an error") {
    const std::string path = "/tmp/cmg_io_off_bad.off";
    {
        std::ofstream f(path);
        f << "OFF\n4 1 0\n"
          << "0 0 0\n1 0 0\n"; // declares 4 verts, supplies 2
    }
    auto r = io::read_off(path);
    CMG_CHECK(!r);
}
