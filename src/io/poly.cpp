// CyberMeshGenerator — TetGen `.poly` and `.smesh` PLC containers.
//
// Both formats share a node section, a hole list, and a region list; they differ
// only in how facets are spelled: a `.poly` facet may hold several polygons plus
// sub-facet holes, whereas a `.smesh` facet is a single polygon on one line with
// an optional trailing marker. Mirrors TetGen's load_poly/save_poly grammar.
#include <iomanip>
#include <istream>
#include <string>
#include <vector>

#include "cmg/io/detail.hpp"
#include "cmg/io/formats.hpp"

namespace cmg::io {
namespace {

using detail::bad;
using detail::next_data_line;
using detail::tokenize;
using detail::to_int;
using detail::to_real;

/// Append tokens from further data lines until `toks` holds at least `need`.
bool ensure_tokens(std::istream& in, std::vector<std::string>& toks,
                   std::size_t need) {
    while (toks.size() < need) {
        std::string line;
        if (!next_data_line(in, line)) return false;
        auto more = tokenize(line);
        toks.insert(toks.end(), more.begin(), more.end());
    }
    return true;
}

/// Derive the sibling `<base>.node` path from a `.poly`/`.smesh` path.
std::string node_sibling(const std::string& path) {
    auto dot = path.find_last_of('.');
    std::string base = (dot == std::string::npos) ? path : path.substr(0, dot);
    return base + ".node";
}

/// Load the point section: inline when the header declares points, otherwise
/// from the companion `.node`. Returns the detected index base on success.
expected<int, MeshError> read_points_into(std::istream& in,
                                          const std::string& path, PLC& plc) {
    auto ns = detail::read_node_section(in);
    if (!ns) return unexpected(ns.error());
    if (!ns->points.empty()) {
        plc.points = std::move(ns->points);
        return ns->index_base;
    }
    // Header declared zero points: read them from the sibling .node file.
    std::ifstream nin;
    if (auto o = detail::open_in(node_sibling(path), nin); !o)
        return unexpected(o.error());
    auto ns2 = detail::read_node_section(nin);
    if (!ns2) return unexpected(ns2.error());
    plc.points = std::move(ns2->points);
    return ns2->index_base;
}

/// Read `count` coordinate triples (each prefixed by an ignored index) as points.
expected<std::vector<Point3>, MeshError> read_coord_list(std::istream& in,
                                                         long count,
                                                         const char* what) {
    std::vector<Point3> out;
    out.reserve(count > 0 ? count : 0);
    for (long i = 0; i < count; ++i) {
        std::string line;
        if (!next_data_line(in, line)) return bad(std::string(what) + ": truncated");
        auto t = tokenize(line);
        if (t.size() < 4) return bad(std::string(what) + ": short record");
        double x, y, z;
        if (!to_real(t[1], x) || !to_real(t[2], y) || !to_real(t[3], z))
            return bad(std::string(what) + ": non-numeric coordinate");
        out.push_back({static_cast<Real>(x), static_cast<Real>(y),
                       static_cast<Real>(z)});
    }
    return out;
}

/// Read one polygon: `<#corners> v1 v2 ...`, verts continuing across lines.
expected<Polygon, MeshError> read_polygon(std::istream& in, int base) {
    std::string line;
    if (!next_data_line(in, line)) return bad(".poly: missing polygon");
    auto t = tokenize(line);
    long nverts = 0;
    if (!to_int(t[0], nverts) || nverts < 1) return bad(".poly: bad corner count");
    if (!ensure_tokens(in, t, 1 + static_cast<std::size_t>(nverts)))
        return bad(".poly: polygon missing vertices");
    Polygon poly;
    poly.vertices.reserve(nverts);
    for (long k = 0; k < nverts; ++k) {
        long v = 0;
        if (!to_int(t[1 + k], v)) return bad(".poly: non-integer vertex");
        poly.vertices.push_back(static_cast<Index>(v - base));
    }
    return poly;
}

/// Read one `.poly` facet: header `<#polygons> [#holes] [marker]`.
expected<Facet, MeshError> read_poly_facet(std::istream& in, int base,
                                           bool markers) {
    std::string line;
    if (!next_data_line(in, line)) return bad(".poly: missing facet");
    auto h = tokenize(line);
    long npoly = 0, nholes = 0, marker = 0;
    if (!to_int(h[0], npoly) || npoly < 1) return bad(".poly: bad polygon count");
    if (h.size() > 1) to_int(h[1], nholes);
    if (markers && h.size() > 2) to_int(h[2], marker);

    Facet f;
    f.marker = static_cast<int>(marker);
    for (long j = 0; j < npoly; ++j) {
        auto p = read_polygon(in, base);
        if (!p) return unexpected(p.error());
        f.polygons.push_back(std::move(p.value()));
    }
    auto holes = read_coord_list(in, nholes, ".poly facet hole");
    if (!holes) return unexpected(holes.error());
    f.holes = std::move(holes.value());
    return f;
}

/// Read one `.smesh` facet: a single line `<#corners> v... [marker]`.
expected<Facet, MeshError> read_smesh_facet(std::istream& in, int base,
                                            bool markers) {
    std::string line;
    if (!next_data_line(in, line)) return bad(".smesh: missing facet");
    auto t = tokenize(line);
    long nverts = 0;
    if (!to_int(t[0], nverts) || nverts < 1) return bad(".smesh: bad corner count");
    std::size_t need = 1 + static_cast<std::size_t>(nverts);
    if (!ensure_tokens(in, t, need)) return bad(".smesh: facet missing vertices");

    Facet f;
    Polygon poly;
    poly.vertices.reserve(nverts);
    for (long k = 0; k < nverts; ++k) {
        long v = 0;
        if (!to_int(t[1 + k], v)) return bad(".smesh: non-integer vertex");
        poly.vertices.push_back(static_cast<Index>(v - base));
    }
    if (markers && t.size() > need) {
        long m = 0;
        to_int(t[need], m);
        f.marker = static_cast<int>(m);
    }
    f.polygons.push_back(std::move(poly));
    return f;
}

/// Read the shared trailing hole and region sections into `plc`.
expected<std::monostate, MeshError> read_holes_regions(std::istream& in,
                                                       PLC& plc) {
    std::string line;
    if (!next_data_line(in, line)) return std::monostate{}; // no hole section
    long nholes = 0;
    to_int(tokenize(line)[0], nholes);
    auto holes = read_coord_list(in, nholes, "hole list");
    if (!holes) return unexpected(holes.error());
    plc.holes = std::move(holes.value());

    if (!next_data_line(in, line)) return std::monostate{}; // regions optional
    long nreg = 0;
    to_int(tokenize(line)[0], nreg);
    for (long i = 0; i < nreg; ++i) {
        if (!next_data_line(in, line)) return bad("region list: truncated");
        auto t = tokenize(line);
        if (t.size() < 5) return bad("region list: short record");
        double x, y, z, attr, vol = -1;
        if (!to_real(t[1], x) || !to_real(t[2], y) || !to_real(t[3], z) ||
            !to_real(t[4], attr))
            return bad("region list: non-numeric field");
        if (t.size() > 5) to_real(t[5], vol);
        plc.regions.push_back(
            {{static_cast<Real>(x), static_cast<Real>(y), static_cast<Real>(z)},
             static_cast<Real>(attr), static_cast<Real>(vol)});
    }
    return std::monostate{};
}

/// Shared driver for `.poly` (smesh=false) and `.smesh` (smesh=true).
expected<PLC, MeshError> read_plc_impl(const std::string& path, bool smesh) {
    std::ifstream in;
    if (auto o = detail::open_in(path, in); !o) return unexpected(o.error());

    PLC plc;
    auto base = read_points_into(in, path, plc);
    if (!base) return unexpected(base.error());
    plc.index_base = (base.value() == 1) ? IndexBase::One : IndexBase::Zero;

    std::string line;
    if (!next_data_line(in, line)) return plc; // no facet section
    auto h = tokenize(line);
    long nfacets = 0, markers = 0;
    to_int(h[0], nfacets);
    if (h.size() > 1) to_int(h[1], markers);
    if (nfacets <= 0) return plc;

    for (long i = 0; i < nfacets; ++i) {
        auto f = smesh ? read_smesh_facet(in, base.value(), markers == 1)
                       : read_poly_facet(in, base.value(), markers == 1);
        if (!f) return unexpected(f.error());
        plc.facets.push_back(std::move(f.value()));
    }
    if (auto hr = read_holes_regions(in, plc); !hr)
        return unexpected(hr.error());
    return plc;
}

/// Emit the trailing hole and region sections (identical in both formats).
void write_holes_regions(std::ostream& out, const PLC& plc, int base) {
    out << plc.holes.size() << '\n';
    for (std::size_t i = 0; i < plc.holes.size(); ++i) {
        const auto& p = plc.holes[i];
        out << (base + static_cast<int>(i)) << ' ' << p.x << ' ' << p.y << ' '
            << p.z << '\n';
    }
    out << plc.regions.size() << '\n';
    for (std::size_t i = 0; i < plc.regions.size(); ++i) {
        const auto& r = plc.regions[i];
        out << (base + static_cast<int>(i)) << ' ' << r.seed.x << ' ' << r.seed.y
            << ' ' << r.seed.z << ' ' << r.attribute << ' ' << r.max_volume
            << '\n';
    }
}

} // namespace

expected<PLC, MeshError> read_poly(const std::string& path) {
    return read_plc_impl(path, /*smesh=*/false);
}

expected<PLC, MeshError> read_smesh(const std::string& path) {
    return read_plc_impl(path, /*smesh=*/true);
}

WriteResult write_poly(const std::string& path, const PLC& plc) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return unexpected(o.error());
    out.precision(17);
    const int base = (plc.index_base == IndexBase::One) ? 1 : 0;

    detail::write_node_section(out, plc.points, {}, base);
    out << plc.facets.size() << " 1\n"; // marker flag always on for round-trip
    for (const auto& f : plc.facets) {
        out << f.polygons.size() << ' ' << f.holes.size() << ' ' << f.marker
            << '\n';
        for (const auto& poly : f.polygons) {
            out << poly.vertices.size();
            for (Index v : poly.vertices) out << ' ' << (v + base);
            out << '\n';
        }
        for (std::size_t j = 0; j < f.holes.size(); ++j) {
            const auto& p = f.holes[j];
            out << (base + static_cast<int>(j)) << ' ' << p.x << ' ' << p.y << ' '
                << p.z << '\n';
        }
    }
    write_holes_regions(out, plc, base);
    return write_ok();
}

WriteResult write_smesh(const std::string& path, const PLC& plc) {
    std::ofstream out;
    if (auto o = detail::open_out(path, out); !o) return unexpected(o.error());
    out.precision(17);
    const int base = (plc.index_base == IndexBase::One) ? 1 : 0;

    detail::write_node_section(out, plc.points, {}, base);
    out << plc.facets.size() << " 1\n"; // marker flag always on for round-trip
    for (const auto& f : plc.facets) {
        if (f.polygons.empty()) {
            out << "0 " << f.marker << '\n';
            continue;
        }
        const auto& poly = f.polygons.front();
        out << poly.vertices.size();
        for (Index v : poly.vertices) out << ' ' << (v + base);
        out << ' ' << f.marker << '\n';
    }
    write_holes_regions(out, plc, base);
    return write_ok();
}

} // namespace cmg::io
