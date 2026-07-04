// CyberMeshGenerator — Delaunay refinement (circumcenter insertion).
#include "cmg/quality/refine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "cmg/api.hpp"
#include "cmg/constrained/tetrahedralize_plc.hpp"
#include "cmg/predicates/robust.hpp"

namespace cmg::quality {
namespace {

constexpr int kMaxRounds = 60;
constexpr std::size_t kDefaultBudget = 200000;

struct V3 { double x, y, z; };
V3 sub(const Point3& a, const Point3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 cross(const V3& a, const V3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double dot(const V3& a, const V3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
double norm2(const V3& a) { return dot(a, a); }

double tet_volume(const Point3& a, const Point3& b, const Point3& c,
                  const Point3& d) {
    return std::fabs(dot(sub(b, a), cross(sub(c, a), sub(d, a)))) / 6.0;
}

// Circumcenter of a,b,c,d. Returns false for a (near-)degenerate/sliver tet.
bool circumcenter(const Point3& a, const Point3& b, const Point3& c,
                  const Point3& d, Point3& out, double& radius) {
    V3 B = sub(b, a), C = sub(c, a), D = sub(d, a);
    double denom = 2.0 * dot(B, cross(C, D));
    double scale = std::sqrt(std::max({norm2(B), norm2(C), norm2(D), 1e-30}));
    if (std::fabs(denom) < 1e-12 * scale * scale * scale) return false;
    V3 num;
    V3 t1 = cross(C, D), t2 = cross(D, B), t3 = cross(B, C);
    num.x = (norm2(B) * t1.x + norm2(C) * t2.x + norm2(D) * t3.x) / denom;
    num.y = (norm2(B) * t1.y + norm2(C) * t2.y + norm2(D) * t3.y) / denom;
    num.z = (norm2(B) * t1.z + norm2(C) * t2.z + norm2(D) * t3.z) / denom;
    out = {static_cast<Real>(a.x + num.x), static_cast<Real>(a.y + num.y),
           static_cast<Real>(a.z + num.z)};
    radius = std::sqrt(norm2(num));
    return true;
}

// p lies inside some tetrahedron of `mesh` (tets stored orient3d(v0..v3) < 0).
bool inside_domain(const Mesh& mesh, const Point3& p) {
    for (const auto& t : mesh.tetrahedra) {
        const Point3 &v0 = mesh.points[t[0]], &v1 = mesh.points[t[1]],
                     &v2 = mesh.points[t[2]], &v3 = mesh.points[t[3]];
        if (robust::orient3d(p, v1, v2, v3) <= 0 &&
            robust::orient3d(v0, p, v2, v3) <= 0 &&
            robust::orient3d(v0, v1, p, v3) <= 0 &&
            robust::orient3d(v0, v1, v2, p) <= 0)
            return true;
    }
    return false;
}

// Re-mesh the current vertex set with refinement OFF (avoids recursion).
expected<Mesh, MeshError> remesh(const std::vector<Point3>& pts, const PLC* plc,
                                 MeshOptions base) {
    base.quality.reset();
    base.max_volume.reset();
    base.sizing = nullptr; // clear ALL refinement triggers to avoid recursion
    if (plc) {
        PLC work = *plc;
        work.points = pts;
        return cdt::tetrahedralize_plc(work, base);
    }
    return delaunay(pts, base);
}

double extent_of(const std::vector<Point3>& pts) {
    Real lo[3] = {pts[0].x, pts[0].y, pts[0].z}, hi[3] = {lo[0], lo[1], lo[2]};
    for (const auto& p : pts)
        for (int a = 0; a < 3; a++) {
            lo[a] = std::min(lo[a], p[a]);
            hi[a] = std::max(hi[a], p[a]);
        }
    return std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2],
                     static_cast<Real>(1)});
}

} // namespace

expected<Mesh, MeshError> refine(std::vector<Point3> points, const PLC* plc,
                                 const MeshOptions& opts) {
    const bool has_vol = opts.max_volume.has_value();
    const double global_vol = has_vol ? *opts.max_volume : 0.0;
    const bool has_sizing = static_cast<bool>(opts.sizing);
    constexpr double kRegularTetVolPerEdge3 = 1.0 / (6.0 * 1.41421356237309515); // 1/(6√2)

    // Per-tetrahedron volume target: the tighter of the global max-volume and the
    // sizing-derived target (h³/6√2) at the tet centroid. Returns <= 0 if the tet
    // is unconstrained here.
    auto target_volume = [&](const Point3& c) -> double {
        double t = has_vol ? global_vol : 0.0;
        if (has_sizing) {
            double h = opts.sizing(c);
            if (h > 0) {
                double sv = h * h * h * kRegularTetVolPerEdge3;
                t = (t > 0) ? std::min(t, sv) : sv;
            }
        }
        return t;
    };
    const std::size_t budget =
        opts.steiner_budget ? static_cast<std::size_t>(*opts.steiner_budget)
                            : kDefaultBudget;
    const std::size_t n0 = points.size();
    const double extent = extent_of(points);
    const double near_eps = 1e-6 * extent;
    const double near_eps2 = near_eps * near_eps;

    // A circumcenter is admissible only if it is finite and not absurdly far from
    // the domain: feeding a huge or non-finite point to the exact predicates
    // overruns their fixed expansion buffers. Sliver tets fail this and fall back
    // to the centroid.
    auto cc_admissible = [&](const Point3& c, double R) {
        return std::isfinite(c.x) && std::isfinite(c.y) && std::isfinite(c.z) &&
               R < 1e6 * extent;
    };

    auto too_close = [&](const Point3& c) {
        for (const auto& p : points)
            if (norm2(sub(c, p)) < near_eps2) return true;
        return false;
    };

    Mesh mesh;
    for (int round = 0; round < kMaxRounds; ++round) {
        auto m = remesh(points, plc, opts);
        if (!m) return m;
        mesh = std::move(*m);

        // Collect over-large tetrahedra (this increment refines on VOLUME only).
        // Insert the circumcenter when it lies inside the domain, else the
        // centroid (always inside the tet, hence the domain), so boundary tets can
        // still be split. Volume refinement is self-limiting: subdivision strictly
        // reduces tetrahedron volume toward the bound and slivers have small volume
        // so they are never targeted. (Shape / radius-edge refinement is deferred:
        // circumcenter insertion on slivers diverges without sliver + encroachment
        // handling, so this increment does not attempt it.)
        if (!has_vol && !has_sizing) break; // nothing to refine against
        struct Bad { double key; Point3 site; };
        std::vector<Bad> bad;
        for (const auto& t : mesh.tetrahedra) {
            const Point3 &a = mesh.points[t[0]], &b = mesh.points[t[1]],
                         &c = mesh.points[t[2]], &d = mesh.points[t[3]];
            Point3 ctr{static_cast<Real>((a.x + b.x + c.x + d.x) / 4),
                       static_cast<Real>((a.y + b.y + c.y + d.y) / 4),
                       static_cast<Real>((a.z + b.z + c.z + d.z) / 4)};
            double bound_vol = target_volume(ctr);
            if (bound_vol <= 0) continue; // unconstrained here
            double vol = tet_volume(a, b, c, d);
            if (vol <= bound_vol) continue;
            Point3 cc;
            double R = 0;
            bool have_cc = circumcenter(a, b, c, d, cc, R) && cc_admissible(cc, R);
            Point3 site = (have_cc && inside_domain(mesh, cc)) ? cc : ctr;
            bad.push_back({vol, site});
        }
        if (bad.empty()) break;
        std::sort(bad.begin(), bad.end(),
                  [](const Bad& x, const Bad& y) { return x.key > y.key; });

        std::size_t added = 0;
        for (const Bad& x : bad) {
            if (points.size() - n0 >= budget) break;
            if (too_close(x.site)) continue; // site is inside the domain by construction
            points.push_back(x.site);
            ++added;
        }
        if (added == 0) break; // no admissible circumcenter -> converged/stuck
    }
    return mesh;
}

} // namespace cmg::quality
