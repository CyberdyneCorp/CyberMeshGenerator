// CyberMeshGenerator — surface simplification via grid vertex clustering.
//
// Bins vertices into a uniform grid over the bounding box, collapses each occupied
// cell to the centroid of its vertices, and re-emits the facet triangles over those
// representatives (dropping degenerate and duplicate triangles). Mirrors the
// Rossignac–Borrel clustering the examples previously did by hand, now in the core.
#include "cmg/simplify/simplify.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace cmg::simplify {
namespace {

// Grid cell key for a vertex: (ix, iy, iz) packed. `ext` is the longest axis extent,
// so the grid is cubic with `grid` cells along that axis (fewer along shorter axes).
std::int64_t cell_key(const Point3& p, const Point3& lo, double inv, int grid) {
    auto clamp = [grid](double f) {
        int i = static_cast<int>(f);
        if (i < 0) i = 0;
        if (i >= grid) i = grid - 1;
        return i;
    };
    std::int64_t ix = clamp((p.x - lo.x) * inv);
    std::int64_t iy = clamp((p.y - lo.y) * inv);
    std::int64_t iz = clamp((p.z - lo.z) * inv);
    return (ix * grid + iy) * static_cast<std::int64_t>(grid) + iz;
}

// Fan-triangulate every facet polygon over the PLC vertices.
std::vector<std::array<int, 3>> facet_triangles(const PLC& plc) {
    std::vector<std::array<int, 3>> tris;
    for (const Facet& f : plc.facets)
        for (const Polygon& poly : f.polygons) {
            const auto& v = poly.vertices;
            for (std::size_t i = 2; i < v.size(); ++i)
                tris.push_back({v[0], v[i - 1], v[i]});
        }
    return tris;
}

} // namespace

expected<PLC, MeshError> simplify(const PLC& in, const SimplifyOptions& opts) {
    if (in.points.empty())
        return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                    "simplify: PLC has no points"});
    if (opts.grid < 1)
        return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                    "simplify: grid must be >= 1"});

    // Bounding box and the longest-axis extent (cubic cells).
    Point3 lo = in.points[0], hi = in.points[0];
    for (const Point3& p : in.points)
        for (int a = 0; a < 3; ++a) {
            if (p[a] < lo[a]) lo[a] = p[a];
            if (p[a] > hi[a]) hi[a] = p[a];
        }
    double ext = 0;
    for (int a = 0; a < 3; ++a) ext = std::max(ext, double(hi[a]) - double(lo[a]));
    if (ext <= 0)
        return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                    "simplify: degenerate (zero-extent) surface"});
    const double inv = opts.grid / ext;

    // Accumulate each occupied cell's vertex centroid; map old vertex -> rep index.
    std::map<std::int64_t, int> cell_to_rep;
    std::vector<std::array<double, 3>> sums;
    std::vector<int> counts;
    std::vector<int> vtx_rep(in.points.size());
    for (std::size_t i = 0; i < in.points.size(); ++i) {
        std::int64_t key = cell_key(in.points[i], lo, inv, opts.grid);
        auto [it, fresh] = cell_to_rep.try_emplace(key, static_cast<int>(sums.size()));
        if (fresh) {
            sums.push_back({0, 0, 0});
            counts.push_back(0);
        }
        int r = it->second;
        sums[r][0] += in.points[i].x;
        sums[r][1] += in.points[i].y;
        sums[r][2] += in.points[i].z;
        counts[r] += 1;
        vtx_rep[i] = r;
    }

    PLC out;
    out.points.reserve(sums.size());
    for (std::size_t r = 0; r < sums.size(); ++r) {
        double n = counts[r];
        out.points.push_back({static_cast<Real>(sums[r][0] / n),
                              static_cast<Real>(sums[r][1] / n),
                              static_cast<Real>(sums[r][2] / n)});
    }

    // Re-emit triangles over representatives, dropping degenerate and duplicate ones.
    std::set<std::array<int, 3>> seen;
    for (const auto& t : facet_triangles(in)) {
        int a = vtx_rep[t[0]], b = vtx_rep[t[1]], c = vtx_rep[t[2]];
        if (a == b || b == c || a == c) continue; // collapsed to a sliver
        std::array<int, 3> key{a, b, c};
        std::sort(key.begin(), key.end());
        if (!seen.insert(key).second) continue; // duplicate facet
        Facet f;
        f.polygons.push_back(Polygon{{a, b, c}});
        out.facets.push_back(std::move(f));
    }
    return out;
}

} // namespace cmg::simplify
