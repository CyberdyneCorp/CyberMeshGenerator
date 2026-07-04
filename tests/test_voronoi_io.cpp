// CyberMeshGenerator — tests for the Voronoi (.v.*) file writer.
#include <fstream>
#include <string>
#include <vector>

#include "cmg/cmg.hpp"
#include "cmg/io/detail.hpp"
#include "cmg/io/voronoi_io.hpp"
#include "harness.hpp"

using namespace cmg;

namespace {

std::vector<Point3> make_cloud() {
    std::vector<Point3> pts;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                pts.push_back({static_cast<Real>(i), static_cast<Real>(j),
                               static_cast<Real>(k)});
    return pts;  // 27 points, > 20
}

/// First whitespace token of the first data line.
std::string first_header_token(const std::string& path) {
    std::ifstream in(path);
    std::string line;
    if (!io::detail::next_data_line(in, line)) return "";
    auto tok = io::detail::tokenize(line);
    return tok.empty() ? "" : tok[0];
}

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

} // namespace

CMG_TEST("voronoi_io: writes .v.node/.v.edge/.v.cell with correct headers") {
    auto cloud = make_cloud();
    auto mesh = delaunay(cloud, MeshOptions{});
    CMG_CHECK(mesh.has_value());

    auto diagram = voronoi::build(*mesh);

    auto w = io::write_voronoi("/tmp/cmg_vtest.node", diagram);
    CMG_CHECK(w.has_value());

    // .v.node header count == number of Voronoi vertices.
    CMG_CHECK(first_header_token("/tmp/cmg_vtest.v.node") ==
              std::to_string(diagram.vertices.size()));

    // .v.edge header's first token == number of edges.
    CMG_CHECK(first_header_token("/tmp/cmg_vtest.v.edge") ==
              std::to_string(diagram.edges.size()));

    // A ray line (containing " -1 ") appears iff the diagram has rays.
    std::string edge_body = read_file("/tmp/cmg_vtest.v.edge");
    bool has_ray_line = edge_body.find(" -1 ") != std::string::npos;
    CMG_CHECK(has_ray_line == (diagram.ray_count() > 0));
}
