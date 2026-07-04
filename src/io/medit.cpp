// CyberMeshGenerator — Medit (.mesh, INRIA) reader/writer.
//
// The Medit ASCII container is keyword-driven: a version/dimension preamble
// followed by "Vertices", "Tetrahedra" and "Triangles" sections in any order,
// each introduced by its keyword and a count. Indices are 1-based; we normalize
// to the project's 0-based internal representation on read and re-emit 1-based on
// write. The whole file is flattened into a whitespace-separated token stream and
// parsed by keyword, so section ordering and unknown keywords are tolerated.
#include <string>
#include <vector>

#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"

namespace cmg::io {

namespace {

using detail::bad;

/// Flatten a Medit file into whitespace/comma-separated tokens, comments stripped.
expected<std::vector<std::string>, MeshError> read_tokens(const std::string& path) {
    std::ifstream in;
    if (auto r = detail::open_in(path, in); !r) return bad(r.error().message);
    std::vector<std::string> tokens;
    std::string line;
    while (detail::next_data_line(in, line)) {
        auto t = detail::tokenize(line);
        tokens.insert(tokens.end(), t.begin(), t.end());
    }
    return tokens;
}

/// Read a count token at `i`, advancing `i` past it. Fails if absent/non-numeric.
bool read_count(const std::vector<std::string>& tk, std::size_t& i, long& out) {
    if (i >= tk.size()) return false;
    if (!detail::to_int(tk[i], out) || out < 0) return false;
    ++i;
    return true;
}

} // namespace

expected<Mesh, MeshError> read_medit(const std::string& path) {
    auto tokens = read_tokens(path);
    if (!tokens) return unexpected(tokens.error());
    const auto& tk = *tokens;

    Mesh mesh;
    mesh.index_base = IndexBase::Zero;

    std::size_t i = 0;
    while (i < tokens->size()) {
        const std::string& kw = tk[i];

        if (kw == "Vertices" || kw == "vertices" || kw == "VERTICES") {
            ++i;
            long n = 0;
            if (!read_count(tk, i, n)) return bad(".mesh: bad Vertices count");
            for (long v = 0; v < n; ++v) {
                if (i + 3 > tk.size()) return bad(".mesh: truncated vertex list");
                double x, y, z;
                if (!detail::to_real(tk[i], x) || !detail::to_real(tk[i + 1], y) ||
                    !detail::to_real(tk[i + 2], z))
                    return bad(".mesh: non-numeric vertex coordinate");
                mesh.points.push_back({static_cast<Real>(x), static_cast<Real>(y),
                                       static_cast<Real>(z)});
                i += 3;
                // Optional reference/marker token.
                long ref = 0;
                if (i < tk.size() && detail::to_int(tk[i], ref)) ++i;
            }
        } else if (kw == "Tetrahedra" || kw == "tetrahedra" || kw == "TETRAHEDRA") {
            ++i;
            long n = 0;
            if (!read_count(tk, i, n)) return bad(".mesh: bad Tetrahedra count");
            for (long e = 0; e < n; ++e) {
                if (i + 4 > tk.size()) return bad(".mesh: truncated tetra list");
                long a, b, c, d;
                if (!detail::to_int(tk[i], a) || !detail::to_int(tk[i + 1], b) ||
                    !detail::to_int(tk[i + 2], c) || !detail::to_int(tk[i + 3], d))
                    return bad(".mesh: non-numeric tetra index");
                mesh.tetrahedra.push_back({static_cast<Index>(a - 1),
                                           static_cast<Index>(b - 1),
                                           static_cast<Index>(c - 1),
                                           static_cast<Index>(d - 1)});
                i += 4;
                long ref = 0;
                if (i < tk.size() && detail::to_int(tk[i], ref)) ++i;
                mesh.tet_markers.push_back(static_cast<int>(ref));
            }
        } else if (kw == "Triangles" || kw == "triangles" || kw == "TRIANGLES") {
            ++i;
            long n = 0;
            if (!read_count(tk, i, n)) return bad(".mesh: bad Triangles count");
            for (long f = 0; f < n; ++f) {
                if (i + 3 > tk.size()) return bad(".mesh: truncated triangle list");
                long a, b, c;
                if (!detail::to_int(tk[i], a) || !detail::to_int(tk[i + 1], b) ||
                    !detail::to_int(tk[i + 2], c))
                    return bad(".mesh: non-numeric triangle index");
                mesh.faces.push_back({static_cast<Index>(a - 1),
                                      static_cast<Index>(b - 1),
                                      static_cast<Index>(c - 1)});
                i += 3;
                long ref = 0;
                if (i < tk.size() && detail::to_int(tk[i], ref)) ++i;
                mesh.face_markers.push_back(static_cast<int>(ref));
            }
        } else {
            // Unknown keyword or stray number (version, dimension, End); skip one.
            ++i;
        }
    }

    if (mesh.points.empty())
        return bad(".mesh: no Vertices section found");

    return mesh;
}

WriteResult write_medit(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto r = detail::open_out(path, out); !r) return bad(r.error().message);

    out << "MeshVersionFormatted 1\n";
    out << "Dimension 3\n";

    out << "Vertices\n" << mesh.points.size() << "\n";
    for (const auto& p : mesh.points)
        out << p.x << ' ' << p.y << ' ' << p.z << " 0\n";

    out << "Tetrahedra\n" << mesh.tetrahedra.size() << "\n";
    for (std::size_t e = 0; e < mesh.tetrahedra.size(); ++e) {
        const auto& t = mesh.tetrahedra[e];
        const int marker = e < mesh.tet_markers.size() ? mesh.tet_markers[e] : 0;
        out << (t[0] + 1) << ' ' << (t[1] + 1) << ' ' << (t[2] + 1) << ' '
            << (t[3] + 1) << ' ' << marker << "\n";
    }

    if (!mesh.faces.empty()) {
        out << "Triangles\n" << mesh.faces.size() << "\n";
        for (std::size_t f = 0; f < mesh.faces.size(); ++f) {
            const auto& tri = mesh.faces[f];
            const int marker = f < mesh.face_markers.size() ? mesh.face_markers[f] : 0;
            out << (tri[0] + 1) << ' ' << (tri[1] + 1) << ' ' << (tri[2] + 1) << ' '
                << marker << "\n";
        }
    }

    out << "End\n";
    if (!out) return bad("cannot write file: " + path);
    return write_ok();
}

} // namespace cmg::io
