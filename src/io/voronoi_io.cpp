// CyberMeshGenerator — Voronoi diagram file output (TetGen .v.* format).
#include "cmg/io/voronoi_io.hpp"

#include <fstream>
#include <string>

#include "cmg/io/detail.hpp"

namespace cmg::io {

namespace {

/// Strip `base`'s extension and append `suffix` (e.g. ".v.node").
std::string derive_path(const std::string& base, const std::string& suffix) {
    auto dot = base.rfind('.');
    std::string stem = (dot == std::string::npos) ? base : base.substr(0, dot);
    return stem + suffix;
}

} // namespace

WriteResult write_voronoi(const std::string& base_path,
                          const voronoi::VoronoiDiagram& diagram,
                          int index_base) {
    // .v.node — Voronoi vertices as a plain point section (no markers).
    {
        std::ofstream out;
        if (auto r = detail::open_out(derive_path(base_path, ".v.node"), out); !r)
            return detail::bad(r.error().message);
        detail::write_node_section(out, diagram.vertices, {}, index_base);
    }

    // .v.edge — one record per Delaunay face; rays carry a direction, v2 = -1.
    {
        std::ofstream out;
        if (auto r = detail::open_out(derive_path(base_path, ".v.edge"), out); !r)
            return detail::bad(r.error().message);
        out << diagram.edges.size() << " 0\n";
        for (std::size_t i = 0; i < diagram.edges.size(); ++i) {
            const voronoi::Edge& e = diagram.edges[i];
            out << (index_base + static_cast<int>(i)) << ' '
                << (index_base + e.v0) << ' ';
            if (e.v1 >= 0) {
                out << (index_base + e.v1) << '\n';
            } else {
                out << "-1 " << e.dir.x << ' ' << e.dir.y << ' ' << e.dir.z
                    << '\n';
            }
        }
    }

    // .v.cell — per-site list of incident Voronoi vertex indices.
    {
        std::ofstream out;
        if (auto r = detail::open_out(derive_path(base_path, ".v.cell"), out); !r)
            return detail::bad(r.error().message);
        out << diagram.cells.size() << "\n";
        for (std::size_t i = 0; i < diagram.cells.size(); ++i) {
            const std::vector<int>& cell = diagram.cells[i];
            out << (index_base + static_cast<int>(i)) << ' ' << cell.size() << ' ';
            for (int v : cell) out << (index_base + v) << ' ';
            out << '\n';
        }
    }

    return write_ok();
}

} // namespace cmg::io
