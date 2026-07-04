// CyberMeshGenerator — TetGen `.face` / `.edge` / `.neigh` reader/writers.
//
// Grammars (mirror TetGen's load_face / save_faces / save_edges / save_neighbors):
//   .face  header:  <#faces> <marker 0|1>
//          record:  <index> n1 n2 n3 [marker]
//   .edge  header:  <#edges> 0
//          record:  <index> a b
//   .neigh header:  <#tets> 4
//          record:  <index> n0 n1 n2 n3   (neighbor tet ids, -1 for hull)
#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"
#include "cmg/io/io.hpp"

namespace cmg::io {

namespace {

/// The file index base carried by a mesh (0 or 1).
inline int base_of(const Mesh& mesh) {
    return mesh.index_base == IndexBase::One ? 1 : 0;
}

} // namespace

expected<std::vector<Triangle>, MeshError> read_face(const std::string& path) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return unexpected(o.error());

    std::string line;
    if (!detail::next_data_line(in, line)) return detail::bad(".face: missing header");
    auto h = detail::tokenize(line);
    if (h.empty()) return detail::bad(".face: empty header");
    long nfaces = 0, mark = 0;
    if (!detail::to_int(h[0], nfaces) || nfaces < 0)
        return detail::bad(".face: invalid face count");
    if (h.size() > 1) detail::to_int(h[1], mark);
    const bool has_markers = (mark == 1);

    // Read raw (file-indexed) corners; the index base is the minimum vertex index.
    std::vector<Triangle> faces;
    faces.reserve(static_cast<std::size_t>(nfaces));
    long detected_base = 0;
    bool have_base = false;
    for (long i = 0; i < nfaces; ++i) {
        if (!detail::next_data_line(in, line))
            return detail::bad(".face: unexpected end of face list");
        auto t = detail::tokenize(line);
        std::size_t need = 4 + (has_markers ? 1u : 0u);
        if (t.size() < need) return detail::bad(".face: short face record");
        Triangle tri{};
        for (int j = 0; j < 3; ++j) {
            long v = 0;
            if (!detail::to_int(t[1 + j], v))
                return detail::bad(".face: non-integer vertex index");
            tri[static_cast<std::size_t>(j)] = static_cast<Index>(v);
            if (!have_base || v < detected_base) { detected_base = v; have_base = true; }
        }
        faces.push_back(tri);
    }

    // Normalize to 0-based indices by subtracting the detected base.
    if (have_base && detected_base != 0) {
        for (auto& tri : faces)
            for (auto& v : tri) v -= static_cast<Index>(detected_base);
    }
    return faces;
}

WriteResult write_face(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = base_of(mesh);
    const bool has_markers = mesh.face_markers.size() == mesh.faces.size() &&
                             !mesh.face_markers.empty();
    out << mesh.faces.size() << ' ' << (has_markers ? 1 : 0) << '\n';
    for (std::size_t i = 0; i < mesh.faces.size(); ++i) {
        const Triangle& f = mesh.faces[i];
        out << (base + static_cast<int>(i)) << ' ' << (base + f[0]) << ' '
            << (base + f[1]) << ' ' << (base + f[2]);
        if (has_markers) out << ' ' << mesh.face_markers[i];
        out << '\n';
    }
    return write_ok();
}

WriteResult write_edge(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = base_of(mesh);
    // Unique undirected edges from the triangle faces (dedupe on sorted pairs).
    std::set<std::pair<int, int>> edges;
    for (const Triangle& f : mesh.faces) {
        for (int j = 0; j < 3; ++j) {
            int a = f[static_cast<std::size_t>(j)];
            int b = f[static_cast<std::size_t>((j + 1) % 3)];
            if (a > b) std::swap(a, b);
            edges.emplace(a, b);
        }
    }

    out << edges.size() << " 0\n";
    int i = 0;
    for (const auto& [a, b] : edges) {
        out << (base + i) << ' ' << (base + a) << ' ' << (base + b) << '\n';
        ++i;
    }
    return write_ok();
}

WriteResult write_neigh(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = base_of(mesh);
    if (mesh.neighbors.empty()) {
        out << "0 4\n";
        return write_ok();
    }

    out << mesh.neighbors.size() << " 4\n";
    for (std::size_t i = 0; i < mesh.neighbors.size(); ++i) {
        const Tetrahedron& n = mesh.neighbors[i];
        out << (base + static_cast<int>(i));
        for (int j = 0; j < 4; ++j) {
            int v = n[static_cast<std::size_t>(j)];
            out << ' ' << (v >= 0 ? base + v : -1);
        }
        out << '\n';
    }
    return write_ok();
}

} // namespace cmg::io
