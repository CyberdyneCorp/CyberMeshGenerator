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
// independent generic-direction rays and take the majority parity. This makes the
// carve robust without a full boundary-adjacency classifier.
bool inside_domain(const Point3& c, const std::vector<Point3>& pts,
                   const std::vector<Tri>& tris) {
    static const double dirs[3][3] = {{0.5773269, 0.5774341, 0.5771914},
                                      {0.3251097, -0.7211021, 0.6119274},
                                      {-0.8017412, 0.2673021, 0.5346203}};
    int inside_votes = 0;
    for (const auto& dir : dirs) {
        int crossings = 0;
        for (const Tri& t : tris)
            if (ray_hits(c, dir, pts[t[0]], pts[t[1]], pts[t[2]])) ++crossings;
        if (crossings & 1) ++inside_votes;
    }
    return inside_votes >= 2;
}

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

    // Keep tetrahedra whose centroid lies inside the domain.
    std::vector<Tetrahedron> kept;
    for (const Tetrahedron& t : dt->tetrahedra) {
        Point3 c = centroid(dt->points[t[0]], dt->points[t[1]],
                            dt->points[t[2]], dt->points[t[3]]);
        if (inside_domain(c, dt->points, tris)) kept.push_back(t);
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
