// CyberMeshGenerator — Medit (.mesh) round-trip and error tests.
#include <cstdio>
#include <fstream>
#include <string>

#include "cmg/core/mesh.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

namespace {

Mesh make_mesh() {
    Mesh m;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
    m.tetrahedra = {{0, 1, 2, 3}, {1, 2, 3, 4}};
    m.tet_markers = {7, 9};
    m.faces = {{0, 1, 2}, {1, 2, 3}};
    m.face_markers = {3, 4};
    m.index_base = IndexBase::Zero;
    return m;
}

} // namespace

CMG_TEST("medit round-trips points, tetrahedra and markers") {
    const std::string path = "/tmp/cmg_io_medit.mesh";
    Mesh in = make_mesh();

    auto w = cmg::io::write_medit(path, in);
    CMG_CHECK(static_cast<bool>(w));

    auto r = cmg::io::read_medit(path);
    CMG_CHECK(static_cast<bool>(r));
    const Mesh& out = *r;

    CMG_CHECK(out.index_base == IndexBase::Zero);
    CMG_CHECK(out.points.size() == in.points.size());
    for (std::size_t i = 0; i < in.points.size(); ++i) {
        CMG_CHECK(out.points[i].x == in.points[i].x);
        CMG_CHECK(out.points[i].y == in.points[i].y);
        CMG_CHECK(out.points[i].z == in.points[i].z);
    }

    CMG_CHECK(out.tetrahedra.size() == in.tetrahedra.size());
    for (std::size_t i = 0; i < in.tetrahedra.size(); ++i)
        CMG_CHECK(out.tetrahedra[i] == in.tetrahedra[i]);
    CMG_CHECK(out.tet_markers == in.tet_markers);

    CMG_CHECK(out.faces.size() == in.faces.size());
    for (std::size_t i = 0; i < in.faces.size(); ++i)
        CMG_CHECK(out.faces[i] == in.faces[i]);
    CMG_CHECK(out.face_markers == in.face_markers);
}

CMG_TEST("medit tolerates section reordering and unknown keywords") {
    const std::string path = "/tmp/cmg_io_medit_reorder.mesh";
    {
        std::ofstream f(path);
        f << "MeshVersionFormatted 1\nDimension 3\n"
          << "Triangles\n1\n1 2 3 5\n"
          << "Corners\n0\n"  // unknown keyword with a count-like token
          << "Vertices\n3\n0 0 0 0\n1 0 0 0\n0 1 0 0\n"
          << "End\n";
    }
    auto r = cmg::io::read_medit(path);
    CMG_CHECK(static_cast<bool>(r));
    CMG_CHECK(r->points.size() == 3);
    CMG_CHECK(r->faces.size() == 1);
    Triangle expected{0, 1, 2};
    CMG_CHECK(r->faces[0] == expected);
    CMG_CHECK(r->face_markers.size() == 1 && r->face_markers[0] == 5);
}

CMG_TEST("medit rejects truncated vertex list") {
    const std::string path = "/tmp/cmg_io_medit_bad.mesh";
    {
        std::ofstream f(path);
        f << "MeshVersionFormatted 1\nDimension 3\n"
          << "Vertices\n4\n0 0 0\n1 0 0\n"; // declares 4, provides 2
    }
    auto r = cmg::io::read_medit(path);
    CMG_CHECK(!r);
}

CMG_TEST("medit read of missing file fails") {
    auto r = cmg::io::read_medit("/tmp/cmg_io_medit_does_not_exist.mesh");
    CMG_CHECK(!r);
}
