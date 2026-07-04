// CyberMeshGenerator — segment (feature-edge) recovery by bisection.
#include "cmg/recover/segments.hpp"

#include <algorithm>
#include <set>

#include "cmg/api.hpp"

namespace cmg::recover {
namespace {

constexpr int kMaxRounds = 40;

std::array<int, 2> ord(int a, int b) { return a < b ? std::array<int, 2>{a, b}
                                                    : std::array<int, 2>{b, a}; }

// All undirected edges of a Delaunay mesh, as sorted index pairs.
std::set<std::array<int, 2>> mesh_edges(const Mesh& m) {
    std::set<std::array<int, 2>> e;
    for (const auto& t : m.tetrahedra)
        for (int i = 0; i < 4; ++i)
            for (int j = i + 1; j < 4; ++j) e.insert(ord(t[i], t[j]));
    return e;
}

Point3 midpoint(const Point3& a, const Point3& b) {
    return {static_cast<Real>((double(a.x) + b.x) / 2),
            static_cast<Real>((double(a.y) + b.y) / 2),
            static_cast<Real>((double(a.z) + b.z) / 2)};
}

} // namespace

std::vector<std::array<int, 2>> facet_segments(const PLC& plc) {
    std::set<std::array<int, 2>> seen;
    for (const Facet& f : plc.facets)
        for (const Polygon& poly : f.polygons) {
            const auto& v = poly.vertices;
            const std::size_t n = v.size();
            if (n < 2) continue;
            for (std::size_t i = 0; i < n; ++i) {
                int a = v[i], b = v[(i + 1) % n];
                if (a != b) seen.insert(ord(a, b));
            }
        }
    return {seen.begin(), seen.end()};
}

SegmentResult recover_segments(std::span<const Point3> points_in,
                               std::span<const std::array<int, 2>> segments,
                               std::size_t budget) {
    SegmentResult res;
    res.points.assign(points_in.begin(), points_in.end());
    std::vector<std::array<int, 2>> work(segments.begin(), segments.end());
    if (work.empty()) { res.complete = true; return res; }

    for (int round = 0; round < kMaxRounds; ++round) {
        auto m = delaunay(res.points, {});
        if (!m) return res; // cannot mesh (e.g. degenerate) -> incomplete
        auto edges = mesh_edges(*m);

        std::vector<std::array<int, 2>> next;
        std::vector<std::array<int, 2>> missing;
        for (const auto& s : work) {
            if (edges.count(ord(s[0], s[1]))) next.push_back(s);
            else missing.push_back(s);
        }
        if (missing.empty()) { res.complete = true; return res; }

        bool progressed = false;
        for (const auto& s : missing) {
            if (res.steiner_added >= budget) { next.push_back(s); continue; }
            int mi = static_cast<int>(res.points.size());
            res.points.push_back(midpoint(res.points[s[0]], res.points[s[1]]));
            ++res.steiner_added;
            next.push_back({s[0], mi});
            next.push_back({mi, s[1]});
            progressed = true;
        }
        work = std::move(next);
        if (!progressed) return res; // budget exhausted, still missing -> incomplete
    }
    return res; // rounds exhausted
}

} // namespace cmg::recover
