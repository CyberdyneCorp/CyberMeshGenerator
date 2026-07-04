// CyberMeshGenerator — round-trip tests for the `.node` point list format.
#include <fstream>
#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/io/formats.hpp"
#include "harness.hpp"

using namespace cmg;

CMG_TEST("node: round-trip points with markers, base 1") {
    const std::string path = "/tmp/cmg_io_node.node";
    std::vector<Point3> pts = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    std::vector<int> markers = {1, 2, 3, 4};

    auto w = io::write_node(path, pts, markers, 1);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_node(path);
    CMG_CHECK(static_cast<bool>(r));
    CMG_CHECK(r->size() == pts.size());
    for (std::size_t i = 0; i < pts.size(); ++i) {
        CMG_CHECK((*r)[i] == pts[i]);
    }
}

CMG_TEST("node: round-trip base 0 without markers") {
    const std::string path = "/tmp/cmg_io_node0.node";
    std::vector<Point3> pts = {{1.5, 2.5, 3.5}, {-1.0, -2.0, -3.0}};
    std::vector<int> markers; // none

    auto w = io::write_node(path, pts, markers, 0);
    CMG_CHECK(static_cast<bool>(w));

    auto r = io::read_node(path);
    CMG_CHECK(static_cast<bool>(r));
    CMG_CHECK(r->size() == 2);
    CMG_CHECK((*r)[0] == pts[0]);
    CMG_CHECK((*r)[1] == pts[1]);
}

CMG_TEST("node: truncated point list is an error") {
    const std::string path = "/tmp/cmg_io_node_bad.node";
    {
        std::ofstream out(path);
        out << "3 3 0 0\n";       // header claims 3 points
        out << "1 0.0 0.0 0.0\n"; // only one supplied
    }
    auto r = io::read_node(path);
    CMG_CHECK(!r);
}

CMG_TEST("node: missing file is an error") {
    auto r = io::read_node("/tmp/cmg_io_node_does_not_exist.node");
    CMG_CHECK(!r);
}
