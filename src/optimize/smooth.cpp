// CyberMeshGenerator — Laplacian smoothing + dihedral-angle quality metric.
#include "cmg/optimize/smooth.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <set>
#include <vector>

#include "cmg/predicates/robust.hpp"

namespace cmg::optimize {
namespace {

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

inline Vec3 sub(const Point3& a, const Point3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}
inline double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline double norm(const Vec3& a) { return std::sqrt(dot(a, a)); }

/// Per-vertex connectivity derived from the tetrahedra.
struct Adjacency {
    std::vector<std::set<int>> neighbors;   ///< edge-neighbor vertex indices
    std::vector<std::vector<int>> incident; ///< incident tetrahedron indices
    std::vector<char> is_boundary;          ///< appears in a boundary face
};

Adjacency build_adjacency(const Mesh& mesh) {
    const std::size_t n = mesh.points.size();
    Adjacency adj;
    adj.neighbors.resize(n);
    adj.incident.resize(n);
    adj.is_boundary.assign(n, 0);

    for (std::size_t ti = 0; ti < mesh.tetrahedra.size(); ++ti) {
        const auto& t = mesh.tetrahedra[ti];
        for (int a = 0; a < 4; ++a) {
            adj.incident[t[a]].push_back(static_cast<int>(ti));
            for (int b = 0; b < 4; ++b)
                if (a != b) adj.neighbors[t[a]].insert(t[b]);
        }
    }
    for (const auto& f : mesh.faces)
        for (int k = 0; k < 3; ++k) adj.is_boundary[f[k]] = 1;
    return adj;
}

/// True iff moving vertex `v` to `prop` keeps every incident tet's stored
/// orientation (orient3d < 0) strictly negative. `pts` holds current positions.
bool move_is_safe(const Mesh& mesh, const std::vector<Point3>& pts,
                  const std::vector<int>& incident, int v,
                  const Point3& prop) {
    for (int ti : incident) {
        const auto& t = mesh.tetrahedra[ti];
        std::array<Point3, 4> q;
        for (int k = 0; k < 4; ++k) q[k] = (t[k] == v) ? prop : pts[t[k]];
        if (!(robust::orient3d(q[0], q[1], q[2], q[3]) < 0.0)) return false;
    }
    return true;
}

/// Minimum dihedral angle (degrees) of a single tetrahedron.
double tet_min_dihedral(const std::array<Point3, 4>& P) {
    constexpr double kPi = 3.14159265358979323846;
    std::array<Vec3, 4> nrm;
    for (int e = 0; e < 4; ++e) {
        const int a = (e + 1) % 4, b = (e + 2) % 4, c = (e + 3) % 4;
        Vec3 nv = cross(sub(P[b], P[a]), sub(P[c], P[a]));
        if (dot(nv, sub(P[e], P[a])) > 0.0) { nv.x = -nv.x; nv.y = -nv.y; nv.z = -nv.z; }
        nrm[e] = nv;
    }
    double m = 180.0;
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
            int k = -1, l = -1;
            for (int mm = 0; mm < 4; ++mm)
                if (mm != i && mm != j) (k < 0 ? k : l) = mm;
            const double na = norm(nrm[k]), nb = norm(nrm[l]);
            if (na == 0.0 || nb == 0.0) continue;
            double cd = dot(nrm[k], nrm[l]) / (na * nb);
            cd = cd > 1.0 ? 1.0 : (cd < -1.0 ? -1.0 : cd);
            m = std::min(m, (kPi - std::acos(cd)) * 180.0 / kPi);
        }
    return m;
}

/// Minimum dihedral over the tets incident to `v`, evaluated with `v` at `at`.
double incident_min_dihedral(const Mesh& mesh, const std::vector<Point3>& pts,
                             const std::vector<int>& incident, int v,
                             const Point3& at) {
    double m = 180.0;
    for (int ti : incident) {
        const auto& t = mesh.tetrahedra[ti];
        std::array<Point3, 4> q;
        for (int k = 0; k < 4; ++k) q[k] = (t[k] == v) ? at : pts[t[k]];
        m = std::min(m, tet_min_dihedral(q));
    }
    return m;
}

} // namespace

Mesh laplacian_smooth(const Mesh& mesh, const SmoothOptions& opts) {
    const Adjacency adj = build_adjacency(mesh);
    std::vector<Point3> pts = mesh.points;
    const int n = static_cast<int>(pts.size());

    // Full move first (uses relaxation), then progressively damped fractions of
    // the way to the target; the largest safe one wins.
    const double fracs[] = {opts.relaxation, 0.5, 0.25, 0.125, 0.0625};

    for (int it = 0; it < opts.iterations; ++it) {
        for (int v = 0; v < n; ++v) {
            if (adj.is_boundary[v] || adj.incident[v].empty()) continue;
            const auto& nbr = adj.neighbors[v];
            if (nbr.empty()) continue;

            Vec3 c{};
            for (int u : nbr) {
                c.x += pts[u].x;
                c.y += pts[u].y;
                c.z += pts[u].z;
            }
            const double m = static_cast<double>(nbr.size());
            const Point3 target{static_cast<Real>(c.x / m), static_cast<Real>(c.y / m),
                                static_cast<Real>(c.z / m)};
            const Point3 cur = pts[v];
            // Smart Laplacian: accept a move only if it neither inverts a tet nor
            // reduces the minimum dihedral angle of the incident tets. This makes
            // smoothing a monotone quality improver (plain Laplacian can create
            // slivers and worsen the worst angle).
            const double before = incident_min_dihedral(mesh, pts, adj.incident[v], v, cur);

            for (double f : fracs) {
                const Point3 prop{static_cast<Real>(cur.x + f * (target.x - cur.x)),
                                  static_cast<Real>(cur.y + f * (target.y - cur.y)),
                                  static_cast<Real>(cur.z + f * (target.z - cur.z))};
                if (move_is_safe(mesh, pts, adj.incident[v], v, prop) &&
                    incident_min_dihedral(mesh, pts, adj.incident[v], v, prop) >=
                        before - 1e-12) {
                    pts[v] = prop;
                    break;
                }
            }
        }
    }

    Mesh out = mesh;
    out.points = std::move(pts);
    return out;
}

double min_dihedral_angle(const Mesh& mesh) {
    double min_deg = 180.0;
    for (const auto& t : mesh.tetrahedra)
        min_deg = std::min(min_deg,
                           tet_min_dihedral({mesh.points[t[0]], mesh.points[t[1]],
                                             mesh.points[t[2]], mesh.points[t[3]]}));
    return min_deg;
}

} // namespace cmg::optimize
