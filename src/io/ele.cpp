// CyberMeshGenerator — TetGen `.ele` tetrahedron list reader/writer.
//
// Grammar (mirrors TetGen's load_tet / save_elements):
//   header: "<#tets> <nodes-per-tet 4|10> <region-attr 0|1>"
//   record: "<idx> n1 n2 n3 n4 [n5..n10] [attr]"
// Only the first four corner indices define the tetrahedron; higher-order nodes
// (for 10-node elements) are parsed but discarded. The index base is auto-detected
// as the minimum corner index observed across all records.
#include <climits>
#include <vector>

#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"

namespace cmg::io {

expected<Mesh, MeshError> read_ele(const std::string& path,
                                   const std::vector<Point3>& points) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return detail::bad(o.error().message);

    std::string line;
    if (!detail::next_data_line(in, line)) return detail::bad(".ele: missing header");
    auto h = detail::tokenize(line);
    long ntets = 0, corners = 4, nattr = 0;
    if (h.empty() || !detail::to_int(h[0], ntets) || ntets < 0)
        return detail::bad(".ele: invalid number of tetrahedra");
    if (h.size() > 1) detail::to_int(h[1], corners);
    if (h.size() > 2) detail::to_int(h[2], nattr);
    if (corners != 4 && corners != 10)
        return detail::bad(".ele: nodes-per-tet must be 4 or 10");

    std::vector<Tetrahedron> tets;
    std::vector<int> markers;
    tets.reserve(static_cast<std::size_t>(ntets));
    long min_corner = LONG_MAX;

    for (long i = 0; i < ntets; ++i) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".ele: unexpected end of tetrahedron list");
        auto t = detail::tokenize(line);
        // <idx> + <corners> corner indices required.
        std::size_t need = 1 + static_cast<std::size_t>(corners);
        if (t.size() < need) return detail::bad(".ele: short tetrahedron record");

        Tetrahedron tet{};
        for (int j = 0; j < 4; ++j) {
            long c = 0;
            if (!detail::to_int(t[1 + static_cast<std::size_t>(j)], c))
                return detail::bad(".ele: non-numeric vertex index");
            tet[static_cast<std::size_t>(j)] = static_cast<int>(c);
            if (c < min_corner) min_corner = c;
        }
        tets.push_back(tet);

        if (nattr >= 1) {
            long m = 0;
            // Attribute column follows all corners; default 0 if absent.
            std::size_t ai = 1 + static_cast<std::size_t>(corners);
            if (ai < t.size()) {
                double v = 0;
                if (detail::to_real(t[ai], v)) m = static_cast<long>(v);
            }
            markers.push_back(static_cast<int>(m));
        }
    }

    int base = (ntets > 0 && (min_corner == 0 || min_corner == 1))
                   ? static_cast<int>(min_corner)
                   : 0;

    Mesh mesh;
    mesh.points = points;
    mesh.index_base = (base == 1) ? IndexBase::One : IndexBase::Zero;
    const long npts = static_cast<long>(points.size());
    for (auto& tet : tets) {
        for (auto& v : tet) {
            long idx = static_cast<long>(v) - base;
            if (idx < 0 || idx >= npts)
                return detail::bad(".ele: vertex index out of range");
            v = static_cast<int>(idx);
        }
    }
    mesh.tetrahedra = std::move(tets);
    if (nattr >= 1) mesh.tet_markers = std::move(markers);
    return mesh;
}

WriteResult write_ele(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = (mesh.index_base == IndexBase::One) ? 1 : 0;
    const bool mk = !mesh.tet_markers.empty();
    out << mesh.tetrahedra.size() << " 4 " << (mk ? 1 : 0) << "\n";
    for (std::size_t i = 0; i < mesh.tetrahedra.size(); ++i) {
        const auto& t = mesh.tetrahedra[i];
        out << (base + static_cast<int>(i));
        for (int j = 0; j < 4; ++j) out << ' ' << (base + t[static_cast<std::size_t>(j)]);
        if (mk) out << ' ' << (i < mesh.tet_markers.size() ? mesh.tet_markers[i] : 0);
        out << '\n';
    }
    if (!out) return detail::bad(".ele: write failed for " + path);
    return write_ok();
}

} // namespace cmg::io
