// CyberMeshGenerator — facet recovery by conforming Delaunay Steiner insertion.
#include "cmg/recover/facets.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <set>

#include "cmg/api.hpp"

namespace cmg::recover {
namespace {

constexpr int kMaxRounds = 40;

using Tri = std::array<int, 3>;
using Edge = std::array<int, 2>;

Tri sorted3(int a, int b, int c) {
    Tri k{a, b, c};
    std::sort(k.begin(), k.end());
    return k;
}

Edge ord2(int a, int b) {
    return a < b ? Edge{a, b} : Edge{b, a};
}

// All triangular faces of a Delaunay mesh, as sorted index triples.
std::set<Tri> mesh_faces(const Mesh& m) {
    std::set<Tri> f;
    for (const auto& t : m.tetrahedra) {
        f.insert(sorted3(t[1], t[2], t[3]));
        f.insert(sorted3(t[0], t[2], t[3]));
        f.insert(sorted3(t[0], t[1], t[3]));
        f.insert(sorted3(t[0], t[1], t[2]));
    }
    return f;
}

// --- double-precision vector helpers (split-point choice is not correctness-
// critical: the exact present/missing test re-checks every child) -------------
struct V3 { double x, y, z; };
V3 to_v(const Point3& p) { return {double(p.x), double(p.y), double(p.z)}; }
V3 sub(const V3& a, const V3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 add(const V3& a, const V3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 scale(const V3& a, double s) { return {a.x * s, a.y * s, a.z * s}; }
double dot(const V3& a, const V3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(const V3& a, const V3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Point3 to_point(const V3& v) {
    return {static_cast<Real>(v.x), static_cast<Real>(v.y), static_cast<Real>(v.z)};
}

// In-plane circumcenter of coplanar a,b,c (design §2). Empty if degenerate.
std::optional<V3> circumcenter(const V3& a, const V3& b, const V3& c) {
    V3 u = sub(b, a), v = sub(c, a), n = cross(u, v);
    double denom = 2.0 * dot(n, n);
    if (denom == 0.0) return std::nullopt; // degenerate (collinear) triangle
    V3 term = add(scale(cross(v, n), dot(u, u)), scale(cross(n, u), dot(v, v)));
    return add(a, scale(term, 1.0 / denom));
}

// Is coplanar point p inside triangle a,b,c (inclusive of the boundary)?
bool point_in_triangle(const V3& a, const V3& b, const V3& c, const V3& p) {
    V3 n = cross(sub(b, a), sub(c, a));
    double s0 = dot(cross(sub(b, a), sub(p, a)), n);
    double s1 = dot(cross(sub(c, b), sub(p, b)), n);
    double s2 = dot(cross(sub(a, c), sub(p, c)), n);
    return s0 >= 0 && s1 >= 0 && s2 >= 0;
}

// O strictly inside the diametral ball of segment (p,q)?
bool encroaches(const V3& o, const V3& p, const V3& q) {
    V3 mid = scale(add(p, q), 0.5);
    V3 d = sub(o, mid), e = sub(q, p);
    return dot(d, d) < dot(e, e) * 0.25;
}

double sq_len(const V3& a, const V3& b) {
    V3 d = sub(a, b);
    return dot(d, d);
}

// Longest edge of subface s, as a sorted vertex pair (tie-break: smaller pair).
Edge longest_edge(const Tri& s, const std::vector<Point3>& pts) {
    V3 p0 = to_v(pts[s[0]]), p1 = to_v(pts[s[1]]), p2 = to_v(pts[s[2]]);
    struct Cand { double len2; Edge e; };
    Cand cs[3] = {{sq_len(p0, p1), ord2(s[0], s[1])},
                  {sq_len(p1, p2), ord2(s[1], s[2])},
                  {sq_len(p2, p0), ord2(s[2], s[0])}};
    Edge best = cs[0].e;
    double bl = cs[0].len2;
    for (int i = 1; i < 3; ++i)
        if (cs[i].len2 > bl || (cs[i].len2 == bl && cs[i].e < best)) {
            bl = cs[i].len2;
            best = cs[i].e;
        }
    return best;
}

// Split-point decision for a missing subface: either an interior circumcenter
// point, or a request to split its longest edge at the shared midpoint.
struct Decision {
    bool interior = false;
    V3 point{};   // valid when interior
    Edge edge{};  // valid when !interior
};

Decision decide(const Tri& s, const std::vector<Point3>& pts,
                const std::set<Edge>& protected_segs) {
    V3 a = to_v(pts[s[0]]), b = to_v(pts[s[1]]), c = to_v(pts[s[2]]);
    auto o = circumcenter(a, b, c);
    bool ok = o && point_in_triangle(a, b, c, *o);
    if (ok)
        for (const Edge& seg : protected_segs)
            if (encroaches(*o, to_v(pts[seg[0]]), to_v(pts[seg[1]]))) { ok = false; break; }
    if (ok) return {true, *o, {}};
    return {false, {}, longest_edge(s, pts)};
}

// The two children of subface s when its edge e is split at vertex `mid`.
std::array<Tri, 2> split_at_edge(const Tri& s, const Edge& e, int mid) {
    int r = s[0] ^ s[1] ^ s[2] ^ e[0] ^ e[1]; // the vertex not on e
    return {Tri{e[0], mid, r}, Tri{mid, e[1], r}};
}

} // namespace

std::vector<Tri> plc_subfaces(const PLC& plc) {
    std::vector<Tri> subs;
    for (const Facet& f : plc.facets)
        for (const Polygon& poly : f.polygons) {
            const auto& v = poly.vertices;
            for (std::size_t i = 2; i < v.size(); ++i)
                subs.push_back({v[0], v[i - 1], v[i]});
        }
    return subs;
}

FacetResult recover_facets(std::span<const Point3> points_in,
                           std::span<const Tri> subfaces,
                           std::size_t budget,
                           std::span<const Edge> segments) {
    FacetResult res;
    res.points.assign(points_in.begin(), points_in.end());
    std::vector<Tri> work(subfaces.begin(), subfaces.end());
    if (work.empty()) { res.complete = true; return res; }

    std::set<Edge> protected_segs;
    for (const Edge& s : segments) protected_segs.insert(ord2(s[0], s[1]));

    for (int round = 0; round < kMaxRounds; ++round) {
        auto m = delaunay(res.points, {});
        if (!m) return res; // cannot mesh -> incomplete
        const std::set<Tri> faces = mesh_faces(*m);

        std::vector<Tri> missing;
        for (const Tri& s : work)
            if (!faces.count(sorted3(s[0], s[1], s[2]))) missing.push_back(s);
        if (missing.empty()) {
            res.complete = true;
            res.subfaces = std::move(work);
            return res;
        }
        std::sort(missing.begin(), missing.end(),
                  [](const Tri& x, const Tri& y) {
                      return sorted3(x[0], x[1], x[2]) < sorted3(y[0], y[1], y[2]);
                  });

        // Phase A: decide per-missing split; edge requests take priority over
        // interior splits on any subface that shares that edge (conformity).
        std::set<Edge> wanted_edges;
        std::map<Tri, V3> interior_pt; // keyed by sorted3
        for (const Tri& s : missing) {
            Decision d = decide(s, res.points, protected_segs);
            if (d.interior) interior_pt.emplace(sorted3(s[0], s[1], s[2]), d.point);
            else wanted_edges.insert(d.edge);
        }
        // An interior subface whose edge got requested elsewhere becomes edge-split.
        for (auto it = interior_pt.begin(); it != interior_pt.end();) {
            const Tri& s = it->first;
            bool overridden = wanted_edges.count(ord2(s[0], s[1])) ||
                              wanted_edges.count(ord2(s[1], s[2])) ||
                              wanted_edges.count(ord2(s[2], s[0]));
            it = overridden ? interior_pt.erase(it) : std::next(it);
        }

        // Phase B: allocate Steiner points under budget (edges first, then interiors).
        std::map<Edge, int> edge_mid;
        for (const Edge& e : wanted_edges) {
            if (res.steiner_added >= budget) break;
            V3 m0 = to_v(res.points[e[0]]), m1 = to_v(res.points[e[1]]);
            edge_mid[e] = static_cast<int>(res.points.size());
            res.points.push_back(to_point(scale(add(m0, m1), 0.5)));
            ++res.steiner_added;
        }
        std::map<Tri, int> interior_idx;
        for (const auto& [key, pt] : interior_pt) {
            if (res.steiner_added >= budget) break;
            interior_idx[key] = static_cast<int>(res.points.size());
            res.points.push_back(to_point(pt));
            ++res.steiner_added;
        }

        // Phase C: rebuild the work-list. Edge splits (any subface incident to an
        // allocated edge) take priority, then interior splits, else keep as-is.
        std::vector<Tri> next;
        bool progressed = false;
        for (const Tri& s : work) {
            const Edge e01 = ord2(s[0], s[1]), e12 = ord2(s[1], s[2]),
                       e20 = ord2(s[2], s[0]);
            Edge hit{};
            bool has_hit = false;
            for (const Edge& e : {e01, e12, e20})
                if (edge_mid.count(e) && (!has_hit || e < hit)) { hit = e; has_hit = true; }
            if (has_hit) {
                auto kids = split_at_edge(s, hit, edge_mid[hit]);
                next.push_back(kids[0]);
                next.push_back(kids[1]);
                progressed = true;
                continue;
            }
            auto ii = interior_idx.find(sorted3(s[0], s[1], s[2]));
            if (ii != interior_idx.end()) {
                int c = ii->second;
                next.push_back({s[0], s[1], c});
                next.push_back({s[1], s[2], c});
                next.push_back({s[2], s[0], c});
                progressed = true;
                continue;
            }
            next.push_back(s);
        }
        work = std::move(next);
        if (!progressed) return res; // budget blocked all progress -> incomplete
    }
    return res; // rounds exhausted -> incomplete
}

} // namespace cmg::recover
