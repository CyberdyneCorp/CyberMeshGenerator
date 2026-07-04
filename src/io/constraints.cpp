// CyberMeshGenerator — .vol (per-tet max volume) and .mtr (per-node sizing) files.
//
// Both are small ASCII sidecar files used by the refinement pass: .vol carries an
// optional maximum-volume constraint per tetrahedron, .mtr a target size per node.
#include <fstream>
#include <string>
#include <vector>

#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"

namespace cmg::io {

WriteResult write_vol(const std::string& path, const std::vector<double>& vols,
                      int index_base) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);
    out << vols.size() << "\n";
    for (std::size_t i = 0; i < vols.size(); ++i)
        out << (index_base + static_cast<int>(i)) << ' ' << vols[i] << "\n";
    return write_ok();
}

expected<std::vector<double>, MeshError> read_vol(const std::string& path,
                                                  std::size_t num_tets) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return detail::bad(o.error().message);

    std::string line;
    if (!detail::next_data_line(in, line)) return detail::bad(".vol: missing header");
    auto h = detail::tokenize(line);
    long count = 0;
    if (h.empty() || !detail::to_int(h[0], count))
        return detail::bad(".vol: malformed header count");
    if (static_cast<std::size_t>(count) != num_tets)
        return detail::bad(".vol: constraint count does not match tetrahedron count");

    std::vector<double> vols(num_tets, 0.0);
    for (long i = 0; i < count; ++i) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".vol: unexpected end of constraint list");
        auto t = detail::tokenize(line);
        if (t.size() < 2) return detail::bad(".vol: short constraint record");
        double v = 0.0;
        if (!detail::to_real(t[1], v)) return detail::bad(".vol: non-numeric volume");
        vols[static_cast<std::size_t>(i)] = v;
    }
    return vols;
}

WriteResult write_mtr(const std::string& path,
                      const std::vector<double>& sizes) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);
    out << sizes.size() << " 1\n";
    for (double s : sizes) out << s << "\n";
    return write_ok();
}

expected<std::vector<double>, MeshError> read_mtr(const std::string& path) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return detail::bad(o.error().message);

    std::string line;
    if (!detail::next_data_line(in, line)) return detail::bad(".mtr: missing header");
    auto h = detail::tokenize(line);
    long nnodes = 0;
    if (h.empty() || !detail::to_int(h[0], nnodes))
        return detail::bad(".mtr: malformed header node count");
    if (nnodes < 0) return detail::bad(".mtr: negative node count");

    std::vector<double> sizes;
    sizes.reserve(static_cast<std::size_t>(nnodes));
    for (long i = 0; i < nnodes; ++i) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".mtr: unexpected end of metric list");
        auto t = detail::tokenize(line);
        if (t.empty()) return detail::bad(".mtr: empty metric record");
        double v = 0.0;
        if (!detail::to_real(t[0], v)) return detail::bad(".mtr: non-numeric size");
        sizes.push_back(v);
    }
    return sizes;
}

} // namespace cmg::io
