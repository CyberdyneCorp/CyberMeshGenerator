// CyberMeshGenerator — Stanford PLY surface (ASCII) reader/writer.
//
// A simplified ASCII-only PLY loader mirroring TetGen's tetgenio::load_ply: it
// parses the header to learn the vertex/face counts and per-vertex property
// count, reads the leading x/y/z of every vertex (extra properties ignored) and
// maps each face to a single-polygon Facet. Binary PLY is rejected.
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/plc.hpp"
#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"
#include "cmg/io/io.hpp"

namespace cmg::io {
namespace {

/// Case-insensitive equality for ASCII header keywords.
bool iequals(const std::string& a, const char* b) {
    std::size_t i = 0;
    for (; i < a.size() && b[i]; ++i) {
        char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
        char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
        if (ca != cb) return false;
    }
    return i == a.size() && b[i] == '\0';
}

/// The relevant numbers extracted from a PLY header.
struct PlyHeader {
    long nverts = 0;
    long nfaces = 0;
    int vprops = 0; ///< per-vertex property count (>= 3 expected)
};

} // namespace

expected<PLC, MeshError> read_ply(const std::string& path) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return detail::bad(o.error().message);

    std::string line;
    if (!detail::next_data_line(in, line)) return detail::bad(".ply: empty file");
    {
        auto t = detail::tokenize(line);
        if (t.empty() || !iequals(t[0], "ply"))
            return detail::bad(".ply: missing 'ply' magic on first line");
    }

    // --- Parse the header ---------------------------------------------------
    PlyHeader hdr;
    enum class Section { None, Vertex, Face } current = Section::None;
    bool endheader = false;
    while (detail::next_data_line(in, line)) {
        auto t = detail::tokenize(line);
        if (t.empty()) continue;
        if (iequals(t[0], "end_header")) { endheader = true; break; }
        if (iequals(t[0], "format")) {
            if (t.size() < 2 || !iequals(t[1], "ascii"))
                return detail::bad(".ply: only ASCII format is supported");
        } else if (iequals(t[0], "element")) {
            if (t.size() < 3) return detail::bad(".ply: malformed 'element' line");
            long n = 0;
            if (!detail::to_int(t[2], n) || n < 0)
                return detail::bad(".ply: bad element count");
            if (iequals(t[1], "vertex")) {
                current = Section::Vertex;
                hdr.nverts = n;
                hdr.vprops = 0;
            } else if (iequals(t[1], "face")) {
                current = Section::Face;
                hdr.nfaces = n;
            } else {
                current = Section::None;
            }
        } else if (iequals(t[0], "property") && current == Section::Vertex) {
            ++hdr.vprops;
        }
    }
    if (!endheader) return detail::bad(".ply: missing 'end_header'");

    // --- Vertices -----------------------------------------------------------
    PLC plc;
    plc.points.reserve(static_cast<std::size_t>(hdr.nverts));
    for (long i = 0; i < hdr.nverts; ++i) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".ply: unexpected end of vertex list");
        auto t = detail::tokenize(line);
        if (t.size() < 3) return detail::bad(".ply: short vertex record");
        double x, y, z;
        if (!detail::to_real(t[0], x) || !detail::to_real(t[1], y) ||
            !detail::to_real(t[2], z))
            return detail::bad(".ply: non-numeric vertex coordinate");
        plc.points.push_back({static_cast<Real>(x), static_cast<Real>(y),
                              static_cast<Real>(z)});
    }

    // --- Faces --------------------------------------------------------------
    plc.facets.reserve(static_cast<std::size_t>(hdr.nfaces));
    long smallest = 0;
    bool have_index = false;
    for (long i = 0; i < hdr.nfaces; ++i) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".ply: unexpected end of face list");
        auto t = detail::tokenize(line);
        if (t.empty()) return detail::bad(".ply: empty face record");
        long nv = 0;
        if (!detail::to_int(t[0], nv) || nv <= 0)
            return detail::bad(".ply: bad face vertex count");
        if (t.size() < static_cast<std::size_t>(1 + nv))
            return detail::bad(".ply: short face record");
        Polygon poly;
        poly.vertices.reserve(static_cast<std::size_t>(nv));
        for (long k = 0; k < nv; ++k) {
            long idx = 0;
            if (!detail::to_int(t[static_cast<std::size_t>(1 + k)], idx))
                return detail::bad(".ply: non-integer face index");
            if (!have_index || idx < smallest) { smallest = idx; have_index = true; }
            poly.vertices.push_back(static_cast<Index>(idx));
        }
        Facet f;
        f.polygons.push_back(std::move(poly));
        f.marker = 0;
        plc.facets.push_back(std::move(f));
    }

    // --- Normalize index base -----------------------------------------------
    int base = (have_index && smallest == 1) ? 1 : 0;
    if (base != 0) {
        for (auto& f : plc.facets)
            for (auto& p : f.polygons)
                for (auto& v : p.vertices) v -= base;
    }
    plc.index_base = (base == 1) ? IndexBase::One : IndexBase::Zero;
    return plc;
}

WriteResult write_ply(const std::string& path, const PLC& plc) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = (plc.index_base == IndexBase::One) ? 1 : 0;
    std::size_t nfaces = 0;
    for (const auto& f : plc.facets) nfaces += f.polygons.size();

    out << "ply\n"
        << "format ascii 1.0\n"
        << "element vertex " << plc.points.size() << "\n"
        << "property float x\n"
        << "property float y\n"
        << "property float z\n"
        << "element face " << nfaces << "\n"
        << "property list uchar int vertex_indices\n"
        << "end_header\n";

    for (const auto& p : plc.points)
        out << p.x << ' ' << p.y << ' ' << p.z << '\n';

    for (const auto& f : plc.facets) {
        for (const auto& poly : f.polygons) {
            out << poly.vertices.size();
            for (Index v : poly.vertices) out << ' ' << (v + base);
            out << '\n';
        }
    }

    if (!out) return detail::bad(".ply: write failed for " + path);
    return write_ok();
}

} // namespace cmg::io
