// CyberMeshGenerator — boundary-conforming tetrahedralization of a PLC.
//
// DT of the PLC vertices, then keep only the tetrahedra whose centroid is inside
// the domain (ray-cast against the boundary facets). Exact (volume-conserving) for
// convex / star-shaped domains; concave boundary conformance and exact facet
// preservation are deferred (see the change design).
#include "cmg/constrained/tetrahedralize_plc.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

#include "cmg/api.hpp"
#include "cmg/region/classify.hpp"

namespace cmg::cdt {
namespace {

using Tri = std::array<int, 3>;

// Fan-triangulate every facet polygon into triangles over the PLC vertices.
std::vector<Tri> boundary_triangles(const PLC& plc) {
    std::vector<Tri> tris;
    for (const Facet& f : plc.facets) {
        for (const Polygon& poly : f.polygons) {
            const auto& v = poly.vertices;
            for (std::size_t i = 2; i < v.size(); ++i)
                tris.push_back({v[0], v[i - 1], v[i]});
        }
    }
    return tris;
}

// Möller–Trumbore ray/triangle intersection; ray from `o` along `d`, forward only.
bool ray_hits(const Point3& o, const double d[3], const Point3& a,
              const Point3& b, const Point3& c) {
    double e1[3] = {b.x - a.x, b.y - a.y, b.z - a.z};
    double e2[3] = {c.x - a.x, c.y - a.y, c.z - a.z};
    double p[3] = {d[1] * e2[2] - d[2] * e2[1], d[2] * e2[0] - d[0] * e2[2],
                   d[0] * e2[1] - d[1] * e2[0]};
    double det = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
    if (std::fabs(det) < 1e-12) return false; // ray parallel to triangle
    double inv = 1.0 / det;
    double t0[3] = {o.x - a.x, o.y - a.y, o.z - a.z};
    double u = (t0[0] * p[0] + t0[1] * p[1] + t0[2] * p[2]) * inv;
    if (u < 0 || u > 1) return false;
    double q[3] = {t0[1] * e1[2] - t0[2] * e1[1], t0[2] * e1[0] - t0[0] * e1[2],
                   t0[0] * e1[1] - t0[1] * e1[0]};
    double v = (d[0] * q[0] + d[1] * q[1] + d[2] * q[2]) * inv;
    if (v < 0 || u + v > 1) return false;
    double t = (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]) * inv;
    return t > 1e-9; // strictly forward
}

// Point-in-domain by ray casting. A single ray can miscount when it grazes a
// shared edge of two boundary triangles (common once refinement packs many tets
// against the boundary, and worse in single precision), so cast three
// independent generic-direction rays and take the majority parity.
constexpr double kRayDirs[3][3] = {{0.5773269, 0.5774341, 0.5771914},
                                   {0.3251097, -0.7211021, 0.6119274},
                                   {-0.8017412, 0.2673021, 0.5346203}};

// Spatial index over the boundary triangles, one uniform 2-D grid per ray direction.
// The carve uses a fixed set of ray directions, so projecting every triangle onto the
// plane orthogonal to a ray lets a query test only the triangles in its own cell:
// moving along the ray does not change the (u,v) projection, so any triangle the ray
// can hit projects onto — and is bucketed into — the query's cell. The crossing count
// is therefore IDENTICAL to testing every triangle (a superset of the true hits is
// tested, and ray_hits rejects the rest), only far cheaper: O(tets) instead of
// O(tets x triangles).
class CarveGrid {
public:
    CarveGrid(const std::vector<Point3>& pts, const std::vector<Tri>& tris)
        : pts_(pts), tris_(tris) {
        int r = 1;
        while (r * r < static_cast<int>(tris.size())) ++r; // r ~ sqrt(#triangles)
        r_ = std::min(r, 256);
        for (int k = 0; k < 3; ++k) build(k);
    }

    // Majority-of-3-rays point-in-domain test, accelerated by the per-direction grid.
    bool inside(const Point3& c) const {
        int votes = 0;
        for (int k = 0; k < 3; ++k) {
            const Dir& g = dir_[k];
            const std::vector<int>& bucket = g.cells[cell_index(g, c)];
            int crossings = 0;
            for (int ti : bucket) {
                const Tri& t = tris_[ti];
                if (ray_hits(c, kRayDirs[k], pts_[t[0]], pts_[t[1]], pts_[t[2]]))
                    ++crossings;
            }
            if (crossings & 1) ++votes;
        }
        return votes >= 2;
    }

private:
    struct Dir {
        double u[3], v[3];                 // orthonormal basis of the projection plane
        double min_u = 0, min_v = 0;       // 2-D bounds of the projected surface
        double inv_u = 0, inv_v = 0;       // coord -> cell scale
        std::vector<std::vector<int>> cells;
    };

    static void basis(const double d[3], double u[3], double v[3]) {
        double n[3] = {d[0], d[1], d[2]};
        double len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        for (int i = 0; i < 3; ++i) n[i] /= len;
        double h[3] = {1, 0, 0};
        if (std::fabs(n[0]) > 0.9) { h[0] = 0; h[1] = 1; }
        u[0] = n[1] * h[2] - n[2] * h[1];
        u[1] = n[2] * h[0] - n[0] * h[2];
        u[2] = n[0] * h[1] - n[1] * h[0];
        double ul = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        for (int i = 0; i < 3; ++i) u[i] /= ul;
        v[0] = n[1] * u[2] - n[2] * u[1];
        v[1] = n[2] * u[0] - n[0] * u[2];
        v[2] = n[0] * u[1] - n[1] * u[0];
    }
    static double proj(const double a[3], const Point3& p) {
        return a[0] * p.x + a[1] * p.y + a[2] * p.z;
    }
    int clampi(int i) const { return i < 0 ? 0 : (i >= r_ ? r_ - 1 : i); }
    int cell_index(const Dir& g, const Point3& p) const {
        int iu = clampi(static_cast<int>((proj(g.u, p) - g.min_u) * g.inv_u));
        int iv = clampi(static_cast<int>((proj(g.v, p) - g.min_v) * g.inv_v));
        return iu * r_ + iv;
    }

    void build(int k) {
        Dir& g = dir_[k];
        basis(kRayDirs[k], g.u, g.v);
        double lou = 1e300, lov = 1e300, hiu = -1e300, hiv = -1e300;
        for (const Point3& p : pts_) {
            double pu = proj(g.u, p), pv = proj(g.v, p);
            lou = std::min(lou, pu); hiu = std::max(hiu, pu);
            lov = std::min(lov, pv); hiv = std::max(hiv, pv);
        }
        double eu = hiu - lou, ev = hiv - lov;
        g.min_u = lou; g.min_v = lov;
        g.inv_u = r_ / (eu > 0 ? eu * 1.0000001 : 1.0); // pad so hi maps to r-1
        g.inv_v = r_ / (ev > 0 ? ev * 1.0000001 : 1.0);
        g.cells.assign(static_cast<std::size_t>(r_) * r_, {});
        for (int ti = 0; ti < static_cast<int>(tris_.size()); ++ti) {
            const Tri& t = tris_[ti];
            int u0 = r_, u1 = 0, v0 = r_, v1 = 0;
            for (int j = 0; j < 3; ++j) {
                int iu = clampi(static_cast<int>((proj(g.u, pts_[t[j]]) - g.min_u) * g.inv_u));
                int iv = clampi(static_cast<int>((proj(g.v, pts_[t[j]]) - g.min_v) * g.inv_v));
                u0 = std::min(u0, iu); u1 = std::max(u1, iu);
                v0 = std::min(v0, iv); v1 = std::max(v1, iv);
            }
            for (int iu = u0; iu <= u1; ++iu)
                for (int iv = v0; iv <= v1; ++iv)
                    g.cells[static_cast<std::size_t>(iu) * r_ + iv].push_back(ti);
        }
    }

    const std::vector<Point3>& pts_;
    const std::vector<Tri>& tris_;
    Dir dir_[3];
    int r_ = 1;
};

Point3 centroid(const Point3& a, const Point3& b, const Point3& c,
                const Point3& d) {
    return {(a.x + b.x + c.x + d.x) / 4, (a.y + b.y + c.y + d.y) / 4,
            (a.z + b.z + c.z + d.z) / 4};
}

std::array<int, 3> sorted3(int a, int b, int c) {
    std::array<int, 3> k{a, b, c};
    std::sort(k.begin(), k.end());
    return k;
}

} // namespace

expected<Mesh, MeshError> tetrahedralize_plc(const PLC& plc,
                                             const MeshOptions& opts) {
    auto dt = delaunay(plc.points, opts);
    if (!dt) return dt;

    const std::vector<Tri> tris = boundary_triangles(plc);
    if (tris.empty())
        return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                    "PLC has facets but no boundary triangles"});

    // Keep tetrahedra whose centroid lies inside the domain, classified through a
    // spatial index over the boundary triangles (identical result to brute force).
    const CarveGrid grid(dt->points, tris);
    std::vector<Tetrahedron> kept;
    for (const Tetrahedron& t : dt->tetrahedra) {
        Point3 c = centroid(dt->points[t[0]], dt->points[t[1]],
                            dt->points[t[2]], dt->points[t[3]]);
        if (grid.inside(c)) kept.push_back(t);
    }
    if (kept.empty())
        return unexpected(MeshError{
            MeshErrorCode::InvalidInput,
            "no tetrahedron lies inside the domain (is the PLC closed?)"});

    // A face bordering exactly one kept tet is a domain boundary face.
    std::map<std::array<int, 3>, std::pair<int, Tetrahedron>> face_count;
    auto add_face = [&](int a, int b, int c) {
        auto key = sorted3(a, b, c);
        auto& e = face_count[key];
        e.first++;
        e.second = {a, b, c, 0};
    };
    for (const Tetrahedron& t : kept) {
        add_face(t[1], t[2], t[3]);
        add_face(t[0], t[2], t[3]);
        add_face(t[0], t[1], t[3]);
        add_face(t[0], t[1], t[2]);
    }

    Mesh out;
    out.index_base = dt->index_base;
    out.points = dt->points;
    out.point_markers.assign(out.points.size(), 0);
    out.tetrahedra = std::move(kept);
    out.tet_markers.assign(out.tetrahedra.size(), 0);
    for (const auto& [key, val] : face_count) {
        if (val.first == 1) {
            out.faces.push_back({val.second[0], val.second[1], val.second[2]});
            out.face_markers.push_back(1);
        }
    }

    // Apply PLC regions/holes: assign material attributes, remove hole-seeded
    // components, optionally auto-label. No-op without regions/holes/label.
    region::apply(out, plc, opts.label_regions);
    return out;
}

} // namespace cmg::cdt
