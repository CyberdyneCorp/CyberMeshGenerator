// CyberMeshGenerator — TetGen-compatible command-line driver.
//
// Splits arguments into leading-dash switches and file arguments, parses the
// switches through MeshOptions::from_switches, reads the last file argument by
// extension, meshes it (delaunay for .node point sets, tetrahedralize for a
// PLC), and writes the TetGen-style `<stem>.1.node` / `.ele` / `.face` outputs.
#include "cmg/cli/cli.hpp"

#include <cctype>
#include <sstream>
#include <string>

#include "cmg/cmg.hpp"
#include "cmg/io/io.hpp"

namespace cmg::cli {

namespace {

/// Lower-case extension (without the dot), or empty if there is none.
std::string extension_of(const std::string& path) {
    auto slash = path.find_last_of("/\\");
    auto dot = path.rfind('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
        return {};
    std::string ext = path.substr(dot + 1);
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

/// Drop the final `.xxx` extension from a path.
std::string strip_extension(const std::string& path) {
    auto slash = path.find_last_of("/\\");
    auto dot = path.rfind('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
        return path;
    return path.substr(0, dot);
}

/// True if `s` ends in a `.<digits>` mesh-number suffix.
bool ends_in_numeric_suffix(const std::string& s) {
    auto dot = s.rfind('.');
    if (dot == std::string::npos || dot + 1 >= s.size()) return false;
    for (std::size_t i = dot + 1; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

/// Derive the output base name from the input path per TetGen conventions.
std::string output_stem(const std::string& input) {
    std::string stem = strip_extension(input);
    // If the remaining stem already carries a `.N` mesh number, drop it so we
    // do not accumulate `.1.1` suffixes.
    if (ends_in_numeric_suffix(stem)) return stem.substr(0, stem.rfind('.'));
    return stem;
}

void usage(std::ostream& out) {
    out << "usage: cmg [switches] input.(node|poly|smesh|stl|off|ply)\n";
}

} // namespace

int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) {
    std::string input;
    std::string switch_str;
    bool want_help = false;

    for (const auto& a : args) {
        if (!a.empty() && a.front() == '-') {
            if (a == "-h" || a == "-?") want_help = true;
            switch_str += a.substr(1); // strip a single leading '-'
        } else {
            input = a; // last non-switch wins
        }
    }

    if (want_help || input.empty()) {
        usage(out);
        return 0;
    }

    MeshOptions opts;
    if (!switch_str.empty()) {
        auto parsed = MeshOptions::from_switches(switch_str);
        if (!parsed) {
            err << parsed.error().message << "\n";
            return 2;
        }
        opts = std::move(parsed).value();
    }

    Mesh mesh;
    if (extension_of(input) == "node") {
        auto pts = io::read_points(input);
        if (!pts) {
            err << pts.error().message << "\n";
            return 1;
        }
        auto result = delaunay(*pts, opts);
        if (!result) {
            err << result.error().message << "\n";
            return 1;
        }
        mesh = std::move(result).value();
    } else {
        auto plc = io::read_plc(input);
        if (!plc) {
            err << plc.error().message << "\n";
            return 1;
        }
        auto result = tetrahedralize(*plc, opts);
        if (!result) {
            err << result.error().message << "\n";
            return 1;
        }
        mesh = std::move(result).value();
    }

    const std::string stem = output_stem(input);
    // Writing `<stem>.1.ele` also emits the companion `<stem>.1.node` (the point
    // list is not a standalone mesh-writer format).
    if (auto w = io::write_mesh(stem + ".1.ele", mesh); !w) {
        err << w.error().message << "\n";
        return 1;
    }
    if (auto w = io::write_mesh(stem + ".1.face", mesh); !w) {
        err << w.error().message << "\n";
        return 1;
    }

    out << "cmg: " << mesh.point_count() << " points, " << mesh.tet_count()
        << " tetrahedra\n";
    return 0;
}

} // namespace cmg::cli
