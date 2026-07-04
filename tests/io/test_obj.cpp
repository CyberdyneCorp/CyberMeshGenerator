// Round-trip and error-path tests for the Wavefront OBJ reader/writer.
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

CMG_TEST("obj: PLC write/read round-trip preserves geometry and connectivity") {
    const std::string path = "/tmp/cmg_io_obj_plc.obj";
    PLC in = make_tetra_plc();

    auto w = io::write_obj(path, in);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_obj(path);
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
}

CMG_TEST("obj: face corners in v/vt/vn form use only the vertex index") {
    const std::string path = "/tmp/cmg_io_obj_slash.obj";
    {
        std::ofstream f(path);
        f << "# comment line\n"
          << "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 0 0 1\n"
          << "v 1 1 0\nv 1 0 1\nv 0 1 1\n"
          << "vt 0 0\nvn 0 0 1\n"
          << "f 1/2/3 4/5/6 7/8/9\n";
    }
    auto r = io::read_obj(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& plc = r.value();
    CMG_CHECK(plc.facets.size() == 1);
    const auto& v = plc.facets.front().polygons.front().vertices;
    CMG_CHECK(v.size() == 3);
    CMG_CHECK(v[0] == 0 && v[1] == 3 && v[2] == 6);
}

CMG_TEST("obj: v//vn and plain-index corners parse to their vertex index") {
    const std::string path = "/tmp/cmg_io_obj_normalonly.obj";
    {
        std::ofstream f(path);
        f << "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
          << "f 1//7 2//8 3//9\n";
    }
    auto r = io::read_obj(path);
    CMG_CHECK(static_cast<bool>(r));
    const auto& v = r.value().facets.front().polygons.front().vertices;
    CMG_CHECK(v.size() == 3);
    CMG_CHECK(v[0] == 0 && v[1] == 1 && v[2] == 2);
}

CMG_TEST("obj: mtllib/usemtl/o/g/s statements are skipped") {
    const std::string path = "/tmp/cmg_io_obj_skips.obj";
    {
        std::ofstream f(path);
        f << "mtllib scene.mtl\no cube\ng group1\ns 1\nusemtl red\n"
          << "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
          << "f 1 2 3\n";
    }
    auto r = io::read_obj(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& plc = r.value();
    CMG_CHECK(plc.points.size() == 3);
    CMG_CHECK(plc.facets.size() == 1);
}

CMG_TEST("obj: mesh triangle faces round-trip through write_obj_mesh") {
    const std::string path = "/tmp/cmg_io_obj_mesh.obj";
    Mesh m;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    m.faces = {{0, 1, 2}};

    auto w = io::write_obj_mesh(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_obj(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& plc = r.value();
    CMG_CHECK(plc.points.size() == 3);
    CMG_CHECK(plc.facets.size() == 1);
    const auto& v = plc.facets.front().polygons.front().vertices;
    CMG_CHECK(v.size() == 3);
    CMG_CHECK(v[0] == 0 && v[1] == 1 && v[2] == 2);
}

CMG_TEST("obj: missing file returns an error") {
    auto r = io::read_obj("/tmp/cmg_io_obj_does_not_exist.obj");
    CMG_CHECK(!r);
}
