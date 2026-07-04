// CyberMeshGenerator — Geomview OFF surface reader/writer.
//
// Grammar (ASCII):
//   [OFF]
//   <#verts> <#faces> <#edges>
//   x y z                       (repeated #verts times)
//   <n> i0 i1 ... i(n-1)        (repeated #faces times)
// '#' begins a comment. The face index base (0 or 1) is auto-detected from the
// smallest referenced vertex index, mirroring TetGen's load_off().
#include "cmg/io/formats.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "cmg/io/detail.hpp"

namespace cmg::io {

expected<PLC, MeshError> read_off(const std::string& path) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return detail::bad(o.error().message);

    std::string line;
    if (!detail::next_data_line(in, line)) return detail::bad(".off: empty file");

    // The header line may be "OFF" alone, "OFF <v> <f> <e>", or just the counts.
    auto tok = detail::tokenize(line);
    std::size_t pos = 0;
    if (!tok.empty() && tok[0] == "OFF") pos = 1;
    if (pos >= tok.size()) {
        // Counts are on the next data line.
        if (!detail::next_data_line(in, line)) return detail::bad(".off: missing counts");
        tok = detail::tokenize(line);
        pos = 0;
    }
    if (tok.size() < pos + 3) return detail::bad(".off: malformed count header");

    long nverts = 0, nfaces = 0, nedges = 0;
    if (!detail::to_int(tok[pos], nverts) || !detail::to_int(tok[pos + 1], nfaces) ||
        !detail::to_int(tok[pos + 2], nedges))
        return detail::bad(".off: non-numeric count header");
    if (nverts < 0 || nfaces < 0) return detail::bad(".off: negative counts");

    PLC plc;
    plc.points.reserve(static_cast<std::size_t>(nverts));
    for (long v = 0; v < nverts; ++v) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".off: unexpected end of vertex list");
        auto t = detail::tokenize(line);
        if (t.size() < 3) return detail::bad(".off: short vertex record");
        double x, y, z;
        if (!detail::to_real(t[0], x) || !detail::to_real(t[1], y) ||
            !detail::to_real(t[2], z))
            return detail::bad(".off: non-numeric vertex coordinate");
        plc.points.push_back({static_cast<Real>(x), static_cast<Real>(y),
                              static_cast<Real>(z)});
    }

    plc.facets.reserve(static_cast<std::size_t>(nfaces));
    long smallest = nverts + 1; // sentinel larger than any valid index
    for (long fidx = 0; fidx < nfaces; ++fidx) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".off: unexpected end of face list");
        auto t = detail::tokenize(line);
        if (t.empty()) return detail::bad(".off: empty face record");
        long n = 0;
        if (!detail::to_int(t[0], n) || n <= 0)
            return detail::bad(".off: invalid polygon vertex count");
        if (t.size() < static_cast<std::size_t>(n) + 1)
            return detail::bad(".off: short polygon record");
        Polygon poly;
        poly.vertices.reserve(static_cast<std::size_t>(n));
        for (long k = 0; k < n; ++k) {
            long idx = 0;
            if (!detail::to_int(t[1 + k], idx))
                return detail::bad(".off: non-numeric polygon index");
            if (idx < smallest) smallest = idx;
            poly.vertices.push_back(static_cast<Index>(idx));
        }
        Facet f;
        f.polygons.push_back(std::move(poly));
        f.marker = 0;
        plc.facets.push_back(std::move(f));
    }

    // Detect and normalize the index base (0 or 1) as TetGen does.
    int base = 0;
    if (nfaces == 0 || smallest == 0) {
        base = 0;
    } else if (smallest == 1) {
        base = 1;
    } else {
        return detail::bad(".off: face indices do not start at 0 or 1");
    }
    if (base != 0) {
        for (auto& f : plc.facets)
            for (auto& p : f.polygons)
                for (auto& v : p.vertices) v -= base;
    }
    plc.index_base = base == 1 ? IndexBase::One : IndexBase::Zero;

    return plc;
}

WriteResult write_off(const std::string& path, const PLC& plc) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = plc.index_base == IndexBase::One ? 1 : 0;

    // Count facets that carry at least one polygon (each contributes one face).
    std::size_t nfaces = 0;
    for (const auto& f : plc.facets)
        if (!f.polygons.empty()) ++nfaces;

    out << "OFF\n";
    out << plc.points.size() << ' ' << nfaces << " 0\n";
    for (const auto& p : plc.points)
        out << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for (const auto& f : plc.facets) {
        if (f.polygons.empty()) continue;
        const auto& poly = f.polygons.front();
        out << poly.vertices.size();
        for (Index v : poly.vertices) out << ' ' << (v + base);
        out << '\n';
    }
    if (!out) return detail::bad("failed writing .off file: " + path);
    return write_ok();
}

WriteResult write_off_mesh(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = mesh.index_base == IndexBase::One ? 1 : 0;

    out << "OFF\n";
    out << mesh.points.size() << ' ' << mesh.faces.size() << " 0\n";
    for (const auto& p : mesh.points)
        out << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for (const auto& tri : mesh.faces)
        out << "3 " << (tri[0] + base) << ' ' << (tri[1] + base) << ' '
            << (tri[2] + base) << '\n';
    if (!out) return detail::bad("failed writing .off file: " + path);
    return write_ok();
}

} // namespace cmg::io
