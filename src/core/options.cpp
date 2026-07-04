// CyberMeshGenerator — TetGen-compatible switch-string parser.
//
// Compatibility layer: maps a TetGen switch string (without the leading dash),
// e.g. "pq1.414a0.1", onto the typed MeshOptions. Covers the foundation subset of
// switches; unknown switches are ignored with no effect (as TetGen warns-and-
// continues), while genuinely incompatible combinations return a ParseError.
// (oracle: tetgen.cxx parse_commandline 3074-3795)
#include "cmg/core/options.hpp"

#include <cctype>
#include <cstdlib>

namespace cmg {

namespace {

// Read a floating-point number starting at `i` (may be empty). Advances `i` past
// the digits/sign/dot/exponent consumed. Returns nullopt if no number is present.
std::optional<Real> read_number(std::string_view s, std::size_t& i) {
    const std::size_t start = i;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
    bool any_digit = false;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) ||
                            s[i] == '.')) {
        if (std::isdigit(static_cast<unsigned char>(s[i]))) any_digit = true;
        ++i;
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        ++i;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
    }
    if (!any_digit) { i = start; return std::nullopt; }
    return static_cast<Real>(std::strtod(std::string(s.substr(start, i - start))
                                             .c_str(),
                                         nullptr));
}

// After a numeric sub-parameter, an optional second one may follow '/' or ','.
std::optional<Real> read_second(std::string_view s, std::size_t& i) {
    if (i < s.size() && (s[i] == '/' || s[i] == ',')) {
        ++i;
        return read_number(s, i);
    }
    return std::nullopt;
}

} // namespace

expected<MeshOptions, ParseError>
MeshOptions::from_switches(std::string_view sw) {
    MeshOptions opts;
    std::size_t i = 0;
    while (i < sw.size()) {
        const char c = sw[i++];
        switch (c) {
            case 'p': opts.plc = true; break;
            case 'Y': opts.preserve_surface = true; break;
            case 'r': opts.reconstruct = true; break;
            case 'w': opts.weighted = true; break;
            case 'c': opts.convex = true; break;
            case 'd': opts.detect_intersections = true; break;
            case 'z': opts.index_base = IndexBase::Zero; break;
            case 'n': opts.emit_neighbors = true; break;
            case 'q': {
                Quality qual;
                if (auto v = read_number(sw, i)) qual.radius_edge = *v;
                if (auto m = read_second(sw, i)) qual.min_dihedral = *m;
                opts.quality = qual;
                break;
            }
            case 'a':
                opts.max_volume = read_number(sw, i); // nullopt => variable volume
                if (!opts.quality) opts.quality = Quality{}; // -a implies -q
                break;
            case 'S':
                if (auto v = read_number(sw, i))
                    opts.steiner_budget = static_cast<int>(*v);
                break;
            case 'T':
                if (auto v = read_number(sw, i)) opts.coplanar_tolerance = *v;
                break;
            case 'X':
                opts.predicate_mode = (i < sw.size() && sw[i] == '1')
                                          ? (++i, PredicateMode::NoStaticFilter)
                                          : PredicateMode::FloatingOnly;
                break;
            case ' ': case '-': break; // tolerate separators / a stray dash
            default: break;            // unknown switch: ignore (TetGen warns)
        }
    }

    // Incompatible combination, per TetGen: -w cannot be used with -p or -r.
    if (opts.weighted && (opts.plc || opts.reconstruct)) {
        return unexpected(ParseError{
            "switch 'w' (weighted Delaunay) cannot be combined with 'p' or 'r'"});
    }
    return opts;
}

} // namespace cmg
