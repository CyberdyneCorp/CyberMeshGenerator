// CyberMeshGenerator — legacy ASCII VTK round-trip tests.
#include <cstdio>
#include <fstream>
#include <string>

#include "cmg/core/mesh.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

CMG_TEST("vtk tetrahedral mesh round-trips through write/read") {
    Mesh m;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
    m.tetrahedra = {{0, 1, 2, 3}, {1, 2, 3, 4}};
    m.index_base = IndexBase::Zero;

    const std::string path = "/tmp/cmg_io_vtk.vtk";
    auto w = io::write_vtk(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_vtk(path);
    CMG_CHECK(static_cast<bool>(r));
    const Mesh& got = r.value();

    CMG_CHECK(got.points.size() == m.points.size());
    for (std::size_t i = 0; i < m.points.size(); ++i) {
        CMG_CHECK(got.points[i].x == m.points[i].x);
        CMG_CHECK(got.points[i].y == m.points[i].y);
        CMG_CHECK(got.points[i].z == m.points[i].z);
    }
    CMG_CHECK(got.tetrahedra.size() == m.tetrahedra.size());
    for (std::size_t i = 0; i < m.tetrahedra.size(); ++i)
        CMG_CHECK(got.tetrahedra[i] == m.tetrahedra[i]);
    CMG_CHECK(got.faces.empty());
}

CMG_TEST("vtk surface (triangle) mesh round-trips when no tets present") {
    Mesh m;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
    m.faces = {{0, 1, 2}, {1, 3, 2}};
    m.index_base = IndexBase::Zero;

    const std::string path = "/tmp/cmg_io_vtk_surf.vtk";
    CMG_CHECK(static_cast<bool>(io::write_vtk(path, m)));

    auto r = io::read_vtk(path);
    CMG_CHECK(static_cast<bool>(r));
    const Mesh& got = r.value();
    CMG_CHECK(got.tetrahedra.empty());
    CMG_CHECK(got.faces.size() == 2);
    CMG_CHECK(got.faces[0] == m.faces[0]);
    CMG_CHECK(got.faces[1] == m.faces[1]);
}

CMG_TEST("vtk one-based indices are normalized to zero-based on read") {
    const std::string path = "/tmp/cmg_io_vtk_one.vtk";
    {
        std::ofstream f(path);
        f << "# vtk DataFile Version 2.0\n";
        f << "one based\n";
        f << "ASCII\n";
        f << "DATASET UNSTRUCTURED_GRID\n";
        f << "POINTS 4 double\n";
        f << "0 0 0\n1 0 0\n0 1 0\n0 0 1\n\n";
        f << "CELLS 1 5\n4 1 2 3 4\n\n";
        f << "CELL_TYPES 1\n10\n";
    }
    auto r = io::read_vtk(path);
    CMG_CHECK(static_cast<bool>(r));
    const Tetrahedron expected{0, 1, 2, 3};
    CMG_CHECK(r.value().tetrahedra.size() == 1);
    CMG_CHECK(r.value().tetrahedra[0] == expected);
}

CMG_TEST("vtk rejects BINARY format") {
    const std::string path = "/tmp/cmg_io_vtk_binary.vtk";
    {
        std::ofstream f(path);
        f << "# vtk DataFile Version 2.0\n";
        f << "binary title\n";
        f << "BINARY\n";
        f << "DATASET UNSTRUCTURED_GRID\n";
    }
    auto r = io::read_vtk(path);
    CMG_CHECK(!r);
}

CMG_TEST("vtk truncated POINTS section returns an error") {
    const std::string path = "/tmp/cmg_io_vtk_trunc.vtk";
    {
        std::ofstream f(path);
        f << "# vtk DataFile Version 2.0\n";
        f << "truncated\n";
        f << "ASCII\n";
        f << "DATASET UNSTRUCTURED_GRID\n";
        f << "POINTS 4 double\n";
        f << "0 0 0\n1 0 0\n"; // only 2 of 4 promised points
    }
    auto r = io::read_vtk(path);
    CMG_CHECK(!r);
}
