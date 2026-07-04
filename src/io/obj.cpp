// CyberMeshGenerator — Wavefront OBJ surface reader/writer.
//
// Grammar (ASCII, the subset TetGen consumes):
//   v  x y z              vertex position           (indexed 1-based in OBJ)
//   vt u [v [w]]          texture coordinate         (read but ignored)
//   vn x y z              vertex normal              (read but ignored)
//   f  c1 c2 ... cn       face, n >= 3 corners; each corner ck is one of
//                           "v", "v/vt", "v/vt/vn", or "v//vn" — only the
//                           leading vertex index is used.
// '#' begins a comment. mtllib/usemtl/o/g/s statements are skipped. OBJ vertex
// indices are 1-based and normalized to 0-based on read.
#include "cmg/io/formats.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "cmg/io/detail.hpp"

namespace cmg::io {

namespace {

/// Extract the leading vertex index from an OBJ face corner ("v", "v/vt",
/// "v/vt/vn", or "v//vn"). Returns false if the vertex field is missing or
/// non-numeric.
bool parse_face_corner(const std::string& corner, long& vertex) {
    std::string first = corner.substr(0, corner.find('/'));
    return detail::to_int(first, vertex);
}

} // namespace

expected<PLC, MeshError> read_obj(const std::string& path) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return detail::bad(o.error().message);

    PLC plc;
    std::string line;
    while (detail::next_data_line(in, line)) {
        auto t = detail::tokenize(line);
        if (t.empty()) continue;
        const std::string& tag = t[0];

        if (tag == "v") {
            if (t.size() < 4) return detail::bad(".obj: short vertex record");
            double x, y, z;
            if (!detail::to_real(t[1], x) || !detail::to_real(t[2], y) ||
                !detail::to_real(t[3], z))
                return detail::bad(".obj: non-numeric vertex coordinate");
            plc.points.push_back({static_cast<Real>(x), static_cast<Real>(y),
                                  static_cast<Real>(z)});
        } else if (tag == "f") {
            if (t.size() < 4) return detail::bad(".obj: face needs at least 3 corners");
            Polygon poly;
            poly.vertices.reserve(t.size() - 1);
            for (std::size_t k = 1; k < t.size(); ++k) {
                long v = 0;
                if (!parse_face_corner(t[k], v))
                    return detail::bad(".obj: malformed face corner: " + t[k]);
                poly.vertices.push_back(static_cast<Index>(v - 1)); // 1- to 0-based
            }
            Facet f;
            f.polygons.push_back(std::move(poly));
            plc.facets.push_back(std::move(f));
        }
        // vt, vn, mtllib, usemtl, o, g, s and any other statement are ignored.
    }

    return plc;
}

WriteResult write_obj(const std::string& path, const PLC& plc) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    for (const auto& p : plc.points)
        out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for (const auto& f : plc.facets) {
        if (f.polygons.empty()) continue;
        out << 'f';
        for (Index v : f.polygons.front().vertices) out << ' ' << (v + 1); // 0- to 1-based
        out << '\n';
    }
    if (!out) return detail::bad("failed writing .obj file: " + path);
    return write_ok();
}

WriteResult write_obj_mesh(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    for (const auto& p : mesh.points)
        out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for (const auto& tri : mesh.faces)
        out << "f " << (tri[0] + 1) << ' ' << (tri[1] + 1) << ' ' << (tri[2] + 1)
            << '\n';
    if (!out) return detail::bad("failed writing .obj file: " + path);
    return write_ok();
}

} // namespace cmg::io
