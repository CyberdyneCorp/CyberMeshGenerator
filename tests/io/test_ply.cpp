// CyberMeshGenerator — ASCII PLY round-trip and error tests.
#include <cstdio>
#include <fstream>
#include <string>

#include "cmg/core/geometry.hpp"
#include "cmg/core/plc.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

namespace {

PLC make_pyramid() {
    PLC plc;
    plc.index_base = IndexBase::Zero;
    plc.points = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0},
        {0.0, 1.0, 0.0}, {0.5, 0.5, 1.0},
    };
    // Base quad + four triangular sides.
    auto add_face = [&](std::vector<Index> vs) {
        Facet f;
        Polygon p;
        p.vertices = std::move(vs);
        f.polygons.push_back(std::move(p));
        plc.facets.push_back(std::move(f));
    };
    add_face({0, 1, 2, 3});
    add_face({0, 1, 4});
    add_face({1, 2, 4});
    add_face({2, 3, 4});
    add_face({3, 0, 4});
    return plc;
}

} // namespace

CMG_TEST("ply: round-trips points and face connectivity") {
    const std::string path = "/tmp/cmg_io_ply.ply";
    PLC out = make_pyramid();
    auto w = io::write_ply(path, out);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_ply(path);
    CMG_CHECK(static_cast<bool>(r));
    const PLC& in = r.value();

    CMG_CHECK(in.points.size() == out.points.size());
    for (std::size_t i = 0; i < out.points.size(); ++i) {
        CMG_CHECK(in.points[i].x == out.points[i].x);
        CMG_CHECK(in.points[i].y == out.points[i].y);
        CMG_CHECK(in.points[i].z == out.points[i].z);
    }

    CMG_CHECK(in.facets.size() == out.facets.size());
    for (std::size_t f = 0; f < out.facets.size(); ++f) {
        const auto& op = out.facets[f].polygons.at(0).vertices;
        const auto& ip = in.facets[f].polygons.at(0).vertices;
        CMG_CHECK(ip.size() == op.size());
        for (std::size_t k = 0; k < op.size(); ++k) CMG_CHECK(ip[k] == op[k]);
    }
    std::remove(path.c_str());
}

CMG_TEST("ply: detects and normalizes 1-based indices") {
    const std::string path = "/tmp/cmg_io_ply_base1.ply";
    {
        std::ofstream o(path);
        o << "ply\n"
             "format ascii 1.0\n"
             "element vertex 3\n"
             "property float x\n"
             "property float y\n"
             "property float z\n"
             "element face 1\n"
             "property list uchar int vertex_indices\n"
             "end_header\n"
             "0 0 0\n"
             "1 0 0\n"
             "0 1 0\n"
             "3 1 2 3\n"; // 1-based indices
    }
    auto r = io::read_ply(path);
    CMG_CHECK(static_cast<bool>(r));
    const auto& verts = r.value().facets.at(0).polygons.at(0).vertices;
    CMG_CHECK(verts.size() == 3);
    CMG_CHECK(verts[0] == 0);
    CMG_CHECK(verts[1] == 1);
    CMG_CHECK(verts[2] == 2);
    std::remove(path.c_str());
}

CMG_TEST("ply: ignores extra vertex properties") {
    const std::string path = "/tmp/cmg_io_ply_extra.ply";
    {
        std::ofstream o(path);
        o << "ply\n"
             "format ascii 1.0\n"
             "element vertex 2\n"
             "property float x\n"
             "property float y\n"
             "property float z\n"
             "property uchar red\n"
             "element face 0\n"
             "property list uchar int vertex_indices\n"
             "end_header\n"
             "1.5 2.5 3.5 255\n"
             "4.0 5.0 6.0 128\n";
    }
    auto r = io::read_ply(path);
    CMG_CHECK(static_cast<bool>(r));
    CMG_CHECK(r.value().points.size() == 2);
    CMG_CHECK(r.value().points[0].x == static_cast<Real>(1.5));
    CMG_CHECK(r.value().points[1].z == static_cast<Real>(6.0));
    std::remove(path.c_str());
}

CMG_TEST("ply: rejects non-ply first line") {
    const std::string path = "/tmp/cmg_io_ply_bad.ply";
    {
        std::ofstream o(path);
        o << "not a ply file\n"
             "format ascii 1.0\n"
             "end_header\n";
    }
    auto r = io::read_ply(path);
    CMG_CHECK(!r);
    std::remove(path.c_str());
}

CMG_TEST("ply: rejects binary format") {
    const std::string path = "/tmp/cmg_io_ply_binary.ply";
    {
        std::ofstream o(path);
        o << "ply\n"
             "format binary_little_endian 1.0\n"
             "element vertex 0\n"
             "end_header\n";
    }
    auto r = io::read_ply(path);
    CMG_CHECK(!r);
    std::remove(path.c_str());
}

CMG_TEST("ply: rejects truncated vertex list") {
    const std::string path = "/tmp/cmg_io_ply_trunc.ply";
    {
        std::ofstream o(path);
        o << "ply\n"
             "format ascii 1.0\n"
             "element vertex 3\n"
             "property float x\n"
             "property float y\n"
             "property float z\n"
             "element face 0\n"
             "property list uchar int vertex_indices\n"
             "end_header\n"
             "0 0 0\n"
             "1 0 0\n"; // only 2 of 3 declared vertices
    }
    auto r = io::read_ply(path);
    CMG_CHECK(!r);
    std::remove(path.c_str());
}
