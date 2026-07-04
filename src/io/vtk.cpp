// CyberMeshGenerator — legacy ASCII VTK (UNSTRUCTURED_GRID) reader/writer.
//
// Reads/writes the TetGen-compatible Legacy VTK container: a fixed 4-line header
// (`# vtk DataFile Version`, title, `ASCII`, `DATASET UNSTRUCTURED_GRID`) followed
// by POINTS, CELLS and CELL_TYPES sections. Tetrahedra are VTK_TETRA (type 10) and
// surface triangles are VTK_TRIANGLE (type 5); other cell types are ignored.
#include <algorithm>
#include <cctype>
#include <ios>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"

namespace cmg::io {

namespace {

// Read the next line that is not blank/whitespace-only. Returns false at EOF.
bool next_nonblank(std::istream& in, std::string& out) {
    std::string line;
    while (std::getline(in, line)) {
        for (char c : line) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                out = line;
                return true;
            }
        }
    }
    return false;
}

// First whitespace/comma-delimited token of a line (empty string if none).
std::string first_token(const std::string& line) {
    auto t = detail::tokenize(line);
    return t.empty() ? std::string{} : t.front();
}

} // namespace

expected<Mesh, MeshError> read_vtk(const std::string& path) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return detail::bad(o.error().message);

    // The 4 fixed header lines are read directly (they are not '#'-comment style).
    std::string l1, l2, l3, l4;
    if (!next_nonblank(in, l1)) return detail::bad(".vtk: missing header");
    if (!next_nonblank(in, l2)) return detail::bad(".vtk: missing title line");
    if (!next_nonblank(in, l3)) return detail::bad(".vtk: missing format line");
    if (!next_nonblank(in, l4)) return detail::bad(".vtk: missing dataset line");

    {
        std::string fmt = first_token(l3);
        if (fmt == "BINARY")
            return detail::bad(".vtk: BINARY format is not supported");
        if (fmt != "ASCII")
            return detail::bad(".vtk: expected ASCII data format");
    }

    Mesh mesh;
    std::vector<std::vector<int>> cells; // raw connectivity per cell (in file base)

    std::string line;
    while (next_nonblank(in, line)) {
        auto tok = detail::tokenize(line);
        if (tok.empty()) continue;
        const std::string& id = tok[0];

        if (id == "POINTS") {
            long npts = 0;
            if (tok.size() < 2 || !detail::to_int(tok[1], npts) || npts < 0)
                return detail::bad(".vtk: malformed POINTS header");
            mesh.points.reserve(static_cast<std::size_t>(npts));
            for (long i = 0; i < npts; ++i) {
                if (!next_nonblank(in, line))
                    return detail::bad(".vtk: unexpected end of POINTS section");
                auto c = detail::tokenize(line);
                double x, y, z;
                if (c.size() < 3 || !detail::to_real(c[0], x) ||
                    !detail::to_real(c[1], y) || !detail::to_real(c[2], z))
                    return detail::bad(".vtk: malformed point coordinate");
                mesh.points.push_back({static_cast<Real>(x), static_cast<Real>(y),
                                       static_cast<Real>(z)});
            }
        } else if (id == "CELLS") {
            long ncells = 0;
            if (tok.size() < 2 || !detail::to_int(tok[1], ncells) || ncells < 0)
                return detail::bad(".vtk: malformed CELLS header");
            cells.reserve(static_cast<std::size_t>(ncells));
            for (long i = 0; i < ncells; ++i) {
                if (!next_nonblank(in, line))
                    return detail::bad(".vtk: unexpected end of CELLS section");
                auto c = detail::tokenize(line);
                long np = 0;
                if (c.empty() || !detail::to_int(c[0], np) || np < 0 ||
                    c.size() < static_cast<std::size_t>(np) + 1)
                    return detail::bad(".vtk: malformed cell record");
                std::vector<int> conn;
                conn.reserve(static_cast<std::size_t>(np));
                for (long k = 0; k < np; ++k) {
                    long v = 0;
                    if (!detail::to_int(c[1 + k], v))
                        return detail::bad(".vtk: non-integer cell index");
                    conn.push_back(static_cast<int>(v));
                }
                cells.push_back(std::move(conn));
            }
        } else if (id == "CELL_TYPES") {
            long ncells = 0;
            if (tok.size() < 2 || !detail::to_int(tok[1], ncells) || ncells < 0)
                return detail::bad(".vtk: malformed CELL_TYPES header");
            if (static_cast<std::size_t>(ncells) != cells.size())
                return detail::bad(".vtk: CELL_TYPES count mismatches CELLS");
            for (long i = 0; i < ncells; ++i) {
                if (!next_nonblank(in, line))
                    return detail::bad(".vtk: unexpected end of CELL_TYPES section");
                long type = 0;
                if (!detail::to_int(first_token(line), type))
                    return detail::bad(".vtk: non-integer cell type");
                const auto& c = cells[static_cast<std::size_t>(i)];
                if (type == 10) { // VTK_TETRA
                    if (c.size() != 4)
                        return detail::bad(".vtk: VTK_TETRA needs 4 indices");
                    mesh.tetrahedra.push_back({c[0], c[1], c[2], c[3]});
                } else if (type == 5) { // VTK_TRIANGLE
                    if (c.size() != 3)
                        return detail::bad(".vtk: VTK_TRIANGLE needs 3 indices");
                    mesh.faces.push_back({c[0], c[1], c[2]});
                }
                // Other cell types are ignored.
            }
        }
        // Unknown section headers (SCALARS, LOOKUP_TABLE, ...) are skipped.
    }

    // Normalize to a 0-based mesh by detecting the smallest referenced index.
    int base = 0;
    bool any = false;
    int smallest = 0;
    for (const auto& t : mesh.tetrahedra)
        for (int v : t) { smallest = any ? std::min(smallest, v) : v; any = true; }
    for (const auto& f : mesh.faces)
        for (int v : f) { smallest = any ? std::min(smallest, v) : v; any = true; }
    if (any && smallest == 1) base = 1;
    if (base != 0) {
        for (auto& t : mesh.tetrahedra)
            for (int& v : t) v -= base;
        for (auto& f : mesh.faces)
            for (int& v : f) v -= base;
    }
    mesh.index_base = IndexBase::Zero;
    return mesh;
}

WriteResult write_vtk(const std::string& path, const Mesh& mesh) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return detail::bad(o.error().message);

    const int base = (mesh.index_base == IndexBase::One) ? 1 : 0;

    out << "# vtk DataFile Version 2.0\n";
    out << "CyberMeshGenerator\n";
    out << "ASCII\n";
    out << "DATASET UNSTRUCTURED_GRID\n";

    out.precision(17);
    out << "POINTS " << mesh.points.size() << " double\n";
    for (const auto& p : mesh.points)
        out << p.x << ' ' << p.y << ' ' << p.z << '\n';
    out << '\n';

    if (!mesh.tetrahedra.empty()) {
        const std::size_t n = mesh.tetrahedra.size();
        out << "CELLS " << n << ' ' << (n * 5) << '\n';
        for (const auto& t : mesh.tetrahedra)
            out << "4 " << (t[0] + base) << ' ' << (t[1] + base) << ' '
                << (t[2] + base) << ' ' << (t[3] + base) << '\n';
        out << '\n';
        out << "CELL_TYPES " << n << '\n';
        for (std::size_t i = 0; i < n; ++i) out << "10\n";
    } else {
        const std::size_t n = mesh.faces.size();
        out << "CELLS " << n << ' ' << (n * 4) << '\n';
        for (const auto& f : mesh.faces)
            out << "3 " << (f[0] + base) << ' ' << (f[1] + base) << ' '
                << (f[2] + base) << '\n';
        out << '\n';
        out << "CELL_TYPES " << n << '\n';
        for (std::size_t i = 0; i < n; ++i) out << "5\n";
    }

    if (!out) return detail::bad(".vtk: write failed for " + path);
    return write_ok();
}

} // namespace cmg::io
