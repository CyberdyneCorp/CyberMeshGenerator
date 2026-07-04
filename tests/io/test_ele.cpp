// CyberMeshGenerator — round-trip tests for the `.ele` tetrahedron format.
#include <fstream>
#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

namespace {

std::vector<Point3> five_points() {
    return {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
}

} // namespace

CMG_TEST("ele round-trip with markers (0-based)") {
    const std::string path = "/tmp/cmg_io_ele_zero.ele";

    Mesh m;
    m.points = five_points();
    m.index_base = IndexBase::Zero;
    m.tetrahedra = {Tetrahedron{0, 1, 2, 3}, Tetrahedron{1, 2, 3, 4}};
    m.tet_markers = {7, 42};

    auto w = cmg::io::write_ele(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = cmg::io::read_ele(path, m.points);
    CMG_CHECK(static_cast<bool>(r));
    const Mesh& got = *r;

    CMG_CHECK(got.tetrahedra.size() == 2);
    CMG_CHECK(got.tetrahedra[0] == (Tetrahedron{0, 1, 2, 3}));
    CMG_CHECK(got.tetrahedra[1] == (Tetrahedron{1, 2, 3, 4}));
    CMG_CHECK(got.tet_markers.size() == 2);
    CMG_CHECK(got.tet_markers[0] == 7);
    CMG_CHECK(got.tet_markers[1] == 42);
    CMG_CHECK(got.index_base == IndexBase::Zero);
    CMG_CHECK(got.points.size() == 5);
}

CMG_TEST("ele base detection (1-based, no markers)") {
    const std::string path = "/tmp/cmg_io_ele_one.ele";

    Mesh m;
    m.points = five_points();
    m.index_base = IndexBase::One;
    m.tetrahedra = {Tetrahedron{0, 1, 2, 3}};

    auto w = cmg::io::write_ele(path, m);
    CMG_CHECK(static_cast<bool>(w));

    auto r = cmg::io::read_ele(path, m.points);
    CMG_CHECK(static_cast<bool>(r));
    const Mesh& got = *r;

    // Written as 1-based, detected as 1-based, normalized back to 0-based.
    CMG_CHECK(got.index_base == IndexBase::One);
    CMG_CHECK(got.tetrahedra.size() == 1);
    CMG_CHECK(got.tetrahedra[0] == (Tetrahedron{0, 1, 2, 3}));
    CMG_CHECK(got.tet_markers.empty());
}

CMG_TEST("ele malformed: vertex index out of range") {
    const std::string path = "/tmp/cmg_io_ele_bad.ele";
    {
        std::ofstream f(path);
        f << "1 4 0\n";
        f << "0 0 1 2 99\n"; // 99 is out of range for a 5-point set
    }
    auto r = cmg::io::read_ele(path, five_points());
    CMG_CHECK(!r);
}

CMG_TEST("ele malformed: truncated record") {
    const std::string path = "/tmp/cmg_io_ele_trunc.ele";
    {
        std::ofstream f(path);
        f << "2 4 0\n";
        f << "0 0 1 2 3\n"; // second record missing entirely
    }
    auto r = cmg::io::read_ele(path, five_points());
    CMG_CHECK(!r);
}
