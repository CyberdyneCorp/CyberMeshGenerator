// CyberMeshGenerator — TetGen `.node` point list reader/writer.
//
// Grammar (mirrors TetGen's load_node / save_nodes):
//   header:  <#points> 3 <#attrs> <marker 0|1>
//   record:  <index> x y z [attrs...] [marker]
#include <fstream>
#include <string>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"
#include "cmg/io/io.hpp"

namespace cmg::io {

expected<std::vector<Point3>, MeshError> read_node(const std::string& path) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return unexpected(o.error());
    auto ns = detail::read_node_section(in);
    if (!ns) return unexpected(ns.error());
    return ns->points;
}

WriteResult write_node(const std::string& path, const std::vector<Point3>& pts,
                       const std::vector<int>& markers, int index_base) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);
    detail::write_node_section(out, pts, markers, index_base);
    return write_ok();
}

} // namespace cmg::io
