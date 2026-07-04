// CyberMeshGenerator — STL (stereolithography) surface I/O.
//
// Reads ASCII or binary STL into a PLC, merging coincident vertices; writes ASCII
// STL from a PLC or a Mesh. Grammar mirrors TetGen's tetgenio::load_stl().
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/plc.hpp"
#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"

namespace cmg::io {

namespace {

using detail::bad;

// Ordering key for merging coincident vertices on their exact coordinate triple.
struct CoordKey {
    double x, y, z;
    bool operator<(const CoordKey& o) const {
        if (x != o.x) return x < o.x;
        if (y != o.y) return y < o.y;
        return z < o.z;
    }
};

// Append `p` to `plc`, returning its shared index (deduplicating coincidents).
int intern_vertex(PLC& plc, std::map<CoordKey, int>& seen, const Point3& p) {
    CoordKey key{static_cast<double>(p.x), static_cast<double>(p.y),
                 static_cast<double>(p.z)};
    auto it = seen.find(key);
    if (it != seen.end()) return it->second;
    int idx = static_cast<int>(plc.points.size());
    plc.points.push_back(p);
    seen.emplace(key, idx);
    return idx;
}

// Turn a flat list of vertices (3 per triangle) into merged PLC facets.
expected<PLC, MeshError> build_plc(const std::vector<Point3>& verts) {
    if (verts.empty() || verts.size() % 3 != 0)
        return bad(".stl: vertex count is not a positive multiple of 3");
    PLC plc;
    plc.index_base = IndexBase::Zero;
    std::map<CoordKey, int> seen;
    plc.facets.reserve(verts.size() / 3);
    for (std::size_t i = 0; i < verts.size(); i += 3) {
        Facet f;
        Polygon poly;
        poly.vertices = {intern_vertex(plc, seen, verts[i]),
                         intern_vertex(plc, seen, verts[i + 1]),
                         intern_vertex(plc, seen, verts[i + 2])};
        f.polygons.push_back(std::move(poly));
        f.marker = 0;
        plc.facets.push_back(std::move(f));
    }
    return plc;
}

// Read the whole file into a byte buffer.
bool slurp(const std::string& path, std::vector<char>& out) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return false;
    std::streamoff sz = in.tellg();
    if (sz < 0) return false;
    out.resize(static_cast<std::size_t>(sz));
    in.seekg(0);
    if (sz > 0) in.read(out.data(), sz);
    return true;
}

float read_le_f32(const char* p) {
    std::uint32_t u;
    std::memcpy(&u, p, 4);
    // File is little-endian; assume host is too (all supported targets).
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

std::uint32_t read_le_u32(const char* p) {
    std::uint32_t u;
    std::memcpy(&u, p, 4);
    return u;
}

expected<PLC, MeshError> read_binary(const std::vector<char>& buf, std::uint32_t n) {
    std::vector<Point3> verts;
    verts.reserve(static_cast<std::size_t>(n) * 3);
    std::size_t off = 84; // 80 header + 4 count
    for (std::uint32_t t = 0; t < n; ++t) {
        const char* rec = buf.data() + off;
        // rec[0..11] = normal (skip), then 3 vertices of 12 bytes each.
        for (int v = 0; v < 3; ++v) {
            const char* vp = rec + 12 + v * 12;
            verts.push_back({static_cast<Real>(read_le_f32(vp)),
                             static_cast<Real>(read_le_f32(vp + 4)),
                             static_cast<Real>(read_le_f32(vp + 8))});
        }
        off += 50;
    }
    return build_plc(verts);
}

expected<PLC, MeshError> read_ascii(const std::string& path) {
    std::ifstream in(path);
    if (!in) return bad("cannot open file for reading: " + path);
    std::vector<Point3> verts;
    std::string line;
    while (detail::next_data_line(in, line)) {
        auto tok = detail::tokenize(line);
        if (tok.empty()) continue;
        if (tok[0] == "vertex" || tok[0] == "VERTEX") {
            if (tok.size() < 4) return bad(".stl: short vertex record");
            double x, y, z;
            if (!detail::to_real(tok[1], x) || !detail::to_real(tok[2], y) ||
                !detail::to_real(tok[3], z))
                return bad(".stl: non-numeric vertex coordinate");
            verts.push_back({static_cast<Real>(x), static_cast<Real>(y),
                             static_cast<Real>(z)});
        }
    }
    return build_plc(verts);
}

// Emit one ASCII facet with a computed normal for triangle (a,b,c).
void write_facet(std::ostream& out, const Point3& a, const Point3& b,
                 const Point3& c) {
    double ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
    double vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
    double nx = uy * vz - uz * vy;
    double ny = uz * vx - ux * vz;
    double nz = ux * vy - uy * vx;
    double len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0) { nx /= len; ny /= len; nz /= len; }
    out << "  facet normal " << nx << ' ' << ny << ' ' << nz << "\n";
    out << "    outer loop\n";
    out << "      vertex " << a.x << ' ' << a.y << ' ' << a.z << "\n";
    out << "      vertex " << b.x << ' ' << b.y << ' ' << b.z << "\n";
    out << "      vertex " << c.x << ' ' << c.y << ' ' << c.z << "\n";
    out << "    endloop\n";
    out << "  endfacet\n";
}

} // namespace

expected<PLC, MeshError> read_stl(const std::string& path) {
    std::vector<char> buf;
    if (!slurp(path, buf)) return bad("cannot open file for reading: " + path);

    // Robust binary detection: size must equal 84 + 50 * count.
    if (buf.size() >= 84) {
        std::uint32_t n = read_le_u32(buf.data() + 80);
        if (buf.size() == 84u + 50ull * n) return read_binary(buf, n);
    }
    return read_ascii(path);
}

WriteResult write_stl(const std::string& path, const PLC& plc) {
    std::ofstream out;
    if (auto r = detail::open_out(path, out); !r) return bad(r.error().message);

    out << "solid cmg\n";
    for (const auto& f : plc.facets) {
        for (const auto& poly : f.polygons) {
            if (poly.vertices.size() < 3) continue;
            auto vat = [&](int k) -> const Point3& {
                return plc.points[static_cast<std::size_t>(poly.vertices[k])];
            };
            write_facet(out, vat(0), vat(1), vat(2));
        }
    }
    out << "endsolid cmg\n";
    if (!out) return bad("failed while writing: " + path);
    return write_ok();
}

WriteResult write_stl_mesh(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto r = detail::open_out(path, out); !r) return bad(r.error().message);

    out << "solid cmg\n";
    for (const auto& tri : mesh.faces) {
        write_facet(out, mesh.points[static_cast<std::size_t>(tri[0])],
                    mesh.points[static_cast<std::size_t>(tri[1])],
                    mesh.points[static_cast<std::size_t>(tri[2])]);
    }
    out << "endsolid cmg\n";
    if (!out) return bad("failed while writing: " + path);
    return write_ok();
}

} // namespace cmg::io
