// CyberMeshGenerator — shared inline helpers for the ASCII file parsers.
//
// Centralizes the TetGen file conventions (comments, whitespace/comma separation,
// 0/1 index base) plus the shared `.node` point section, so every format parser
// behaves identically. Header-only and inline; no cross-format coupling.
#pragma once

#include <charconv>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/io/io.hpp"

namespace cmg::io::detail {

/// Remove a `#…` comment (to end of line) from `line`, in place.
inline void strip_comment(std::string& line) {
    auto h = line.find('#');
    if (h != std::string::npos) line.erase(h);
}

/// Read the next non-blank, comment-stripped line into `out`. Returns false at EOF.
inline bool next_data_line(std::istream& in, std::string& out) {
    std::string line;
    while (std::getline(in, line)) {
        strip_comment(line);
        bool has = false;
        for (char c : line)
            if (!std::isspace(static_cast<unsigned char>(c))) { has = true; break; }
        if (has) { out = line; return true; }
    }
    return false;
}

/// Split on whitespace and commas.
inline std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tok;
    std::string cur;
    for (char c : line) {
        if (std::isspace(static_cast<unsigned char>(c)) || c == ',') {
            if (!cur.empty()) { tok.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) tok.push_back(cur);
    return tok;
}

/// Parse a double from a token; returns false on malformed input.
inline bool to_real(const std::string& s, double& out) {
    try { std::size_t n; out = std::stod(s, &n); return n == s.size(); }
    catch (...) { return false; }
}

/// Parse an int from a token; returns false on malformed input.
inline bool to_int(const std::string& s, long& out) {
    try { std::size_t n; out = std::stol(s, &n); return n == s.size(); }
    catch (...) { return false; }
}

inline unexpected<MeshError> bad(std::string msg) {
    return unexpected(MeshError{MeshErrorCode::InvalidInput, std::move(msg)});
}

/// Open a file for reading, or return a MeshError.
inline expected<std::monostate, MeshError> open_in(const std::string& path,
                                                   std::ifstream& in) {
    in.open(path);
    if (!in) return bad("cannot open file for reading: " + path);
    return std::monostate{};
}

/// Open a file for writing, or return a MeshError.
inline expected<std::monostate, MeshError> open_out(const std::string& path,
                                                    std::ofstream& out) {
    out.open(path);
    if (!out) return bad("cannot open file for writing: " + path);
    return std::monostate{};
}

/// The parsed `.node` point section (shared by .node, .poly, .smesh).
struct NodeSection {
    std::vector<Point3> points;
    std::vector<std::vector<double>> attributes; ///< per point (may be empty)
    std::vector<int> markers;                     ///< per point (empty if none)
    int index_base = 0;                           ///< detected 0 or 1
    int num_attributes = 0;
    bool has_markers = false;
};

/// Read a `.node`-format point section from an already-open stream, starting at
/// its header line `<#pts> <dim> <#attrs> <marker>`. Indices are normalized so
/// `points[i]` is the point whose file index is `index_base + i`.
inline expected<NodeSection, MeshError> read_node_section(std::istream& in) {
    std::string line;
    if (!next_data_line(in, line)) return bad(".node: missing header");
    auto h = tokenize(line);
    if (h.size() < 1) return bad(".node: empty header");
    long npts = 0, dim = 3, nattr = 0, mark = 0;
    to_int(h[0], npts);
    if (h.size() > 1) to_int(h[1], dim);
    if (h.size() > 2) to_int(h[2], nattr);
    if (h.size() > 3) to_int(h[3], mark);
    if (npts < 0 || dim != 3) return bad(".node: header must declare dimension 3");

    NodeSection ns;
    ns.num_attributes = static_cast<int>(nattr);
    ns.has_markers = (mark == 1);
    ns.points.reserve(npts);
    for (long i = 0; i < npts; ++i) {
        if (!next_data_line(in, line))
            return bad(".node: unexpected end of point list");
        auto t = tokenize(line);
        // <idx> x y z [attrs...] [marker]
        std::size_t need = 4 + static_cast<std::size_t>(nattr) + (mark == 1 ? 1 : 0);
        if (t.size() < need) return bad(".node: short point record");
        if (i == 0) { long idx0 = 0; to_int(t[0], idx0); ns.index_base = int(idx0); }
        double x, y, z;
        if (!to_real(t[1], x) || !to_real(t[2], y) || !to_real(t[3], z))
            return bad(".node: non-numeric coordinate");
        ns.points.push_back({static_cast<Real>(x), static_cast<Real>(y),
                             static_cast<Real>(z)});
        if (nattr > 0) {
            std::vector<double> a;
            for (long k = 0; k < nattr; ++k) {
                double v = 0; to_real(t[4 + k], v); a.push_back(v);
            }
            ns.attributes.push_back(std::move(a));
        }
        if (mark == 1) {
            long m = 0; to_int(t.back(), m); ns.markers.push_back(int(m));
        }
    }
    return ns;
}

/// Write a `.node`-format point section to a stream.
inline void write_node_section(std::ostream& out,
                               const std::vector<Point3>& pts,
                               const std::vector<int>& markers, int base) {
    const bool mk = markers.size() == pts.size();
    out << pts.size() << " 3 0 " << (mk ? 1 : 0) << "\n";
    for (std::size_t i = 0; i < pts.size(); ++i) {
        out << (base + static_cast<int>(i)) << ' ' << pts[i].x << ' ' << pts[i].y
            << ' ' << pts[i].z;
        if (mk) out << ' ' << markers[i];
        out << '\n';
    }
}

} // namespace cmg::io::detail
