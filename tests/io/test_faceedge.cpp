// Round-trip tests for the .face / .edge / .neigh reader-writers.
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

namespace {

Mesh make_mesh(IndexBase base) {
    Mesh m;
    m.index_base = base;
    m.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    // Four boundary faces of a tetrahedron (0-based internal indices).
    m.faces = {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}};
    m.face_markers = {10, 20, 30, 40};
    m.neighbors = {{1, 2, 3, -1}, {0, -1, 3, 2}, {-1, 0, 1, 3}, {2, 1, 0, -1}};
    return m;
}

} // namespace

CMG_TEST("face round-trip preserves connectivity and markers (base 0)") {
    const std::string path = "/tmp/cmg_io_faceedge0.face";
    Mesh m = make_mesh(IndexBase::Zero);

    auto w = cmg::io::write_face(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = cmg::io::read_face(path);
    CMG_CHECK(static_cast<bool>(r));
    const auto& faces = *r;
    CMG_CHECK(faces.size() == m.faces.size());
    for (std::size_t i = 0; i < faces.size(); ++i) CMG_CHECK(faces[i] == m.faces[i]);
}

CMG_TEST("face round-trip normalizes a 1-based file back to 0-based") {
    const std::string path = "/tmp/cmg_io_faceedge1.face";
    Mesh m = make_mesh(IndexBase::One);

    auto w = cmg::io::write_face(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = cmg::io::read_face(path);
    CMG_CHECK(static_cast<bool>(r));
    const auto& faces = *r;
    CMG_CHECK(faces.size() == m.faces.size());
    // read_face subtracts the detected base, so the result matches the 0-based mesh.
    for (std::size_t i = 0; i < faces.size(); ++i) CMG_CHECK(faces[i] == m.faces[i]);
}

CMG_TEST("edge writer emits unique undirected edges") {
    const std::string path = "/tmp/cmg_io_faceedge.edge";
    Mesh m = make_mesh(IndexBase::Zero);

    auto w = cmg::io::write_edge(path, m);
    CMG_CHECK(static_cast<bool>(w));

    std::ifstream in(path);
    long nedges = -1, flag = -1;
    in >> nedges >> flag;
    // The tetrahedron's 4 faces span exactly its 6 unique edges.
    CMG_CHECK(nedges == 6);
    CMG_CHECK(flag == 0);

    std::set<std::pair<int, int>> seen;
    for (long i = 0; i < nedges; ++i) {
        long idx, a, b;
        in >> idx >> a >> b;
        if (a > b) std::swap(a, b);
        seen.emplace(static_cast<int>(a), static_cast<int>(b));
    }
    CMG_CHECK(seen.size() == 6);
}

CMG_TEST("neigh writer preserves adjacency with base offset") {
    const std::string path = "/tmp/cmg_io_faceedge.neigh";
    Mesh m = make_mesh(IndexBase::One);

    auto w = cmg::io::write_neigh(path, m);
    CMG_CHECK(static_cast<bool>(w));

    std::ifstream in(path);
    long ntets = -1, cols = -1;
    in >> ntets >> cols;
    CMG_CHECK(ntets == 4);
    CMG_CHECK(cols == 4);

    // First tet's neighbors {1,2,3,-1} written with base 1 -> {2,3,4,-1}.
    long idx, n0, n1, n2, n3;
    in >> idx >> n0 >> n1 >> n2 >> n3;
    CMG_CHECK(idx == 1);
    CMG_CHECK(n0 == 2);
    CMG_CHECK(n1 == 3);
    CMG_CHECK(n2 == 4);
    CMG_CHECK(n3 == -1);
}

CMG_TEST("empty neighbors yields a zero-count header") {
    const std::string path = "/tmp/cmg_io_faceedge_empty.neigh";
    Mesh m;
    auto w = cmg::io::write_neigh(path, m);
    CMG_CHECK(static_cast<bool>(w));

    std::ifstream in(path);
    long ntets = -1, cols = -1;
    in >> ntets >> cols;
    CMG_CHECK(ntets == 0);
    CMG_CHECK(cols == 4);
}

CMG_TEST("truncated face file returns an error") {
    const std::string path = "/tmp/cmg_io_faceedge_bad.face";
    {
        std::ofstream out(path);
        out << "3 0\n";       // declares 3 faces...
        out << "1 0 1 2\n";   // ...but supplies only one
    }
    auto r = cmg::io::read_face(path);
    CMG_CHECK(!r);
}
