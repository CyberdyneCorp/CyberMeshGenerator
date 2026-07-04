// CyberMeshGenerator — Voronoi diagram construction.
#include "cmg/voronoi/voronoi.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

#include "cmg/core/circumcenter.hpp"

namespace cmg::voronoi {
namespace {

std::array<int, 3> sorted3(int a, int b, int c) {
    std::array<int, 3> k{a, b, c};
    std::sort(k.begin(), k.end());
    return k;
}

// Face record: the face's vertices (in the first owner's order), the first
// owner's opposite (4th) vertex, and the (up to two) owning tetrahedra.
struct FaceRec {
    std::array<int, 3> fv;
    int opp;
    int t0;
    int t1 = -1;
};

Point3 centroid(const Mesh& m, const Tetrahedron& t) {
    return {(m.points[t[0]].x + m.points[t[1]].x + m.points[t[2]].x + m.points[t[3]].x) / 4,
            (m.points[t[0]].y + m.points[t[1]].y + m.points[t[2]].y + m.points[t[3]].y) / 4,
            (m.points[t[0]].z + m.points[t[1]].z + m.points[t[2]].z + m.points[t[3]].z) / 4};
}

// Orthocenter (weighted circumcenter) of a tetrahedron: the point c with
// |c-p_i|^2 - w_i equal for all four vertices. Solves the 3x3 linear system from
// the pairwise power-equality equations. Returns false if degenerate.
bool orthocenter(const Point3 p[4], const double w[4], Point3& out) {
    double A[3][3], rhs[3];
    auto h = [&](int k) {
        return double(p[k].x) * p[k].x + double(p[k].y) * p[k].y +
               double(p[k].z) * p[k].z - w[k];
    };
    double h0 = h(0);
    for (int i = 0; i < 3; ++i) {
        A[i][0] = double(p[i + 1].x) - p[0].x;
        A[i][1] = double(p[i + 1].y) - p[0].y;
        A[i][2] = double(p[i + 1].z) - p[0].z;
        rhs[i] = (h(i + 1) - h0) / 2.0;
    }
    double det = A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) -
                 A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
                 A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
    if (std::fabs(det) < 1e-300) return false;
    // Cramer's rule.
    auto solve = [&](int col) {
        double M[3][3];
        for (int r = 0; r < 3; ++r)
            for (int cc = 0; cc < 3; ++cc) M[r][cc] = (cc == col) ? rhs[r] : A[r][cc];
        return (M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) -
                M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
                M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0])) /
               det;
    };
    double cx = solve(0), cy = solve(1), cz = solve(2);
    out = {static_cast<Real>(cx), static_cast<Real>(cy), static_cast<Real>(cz)};
    return std::isfinite(cx) && std::isfinite(cy) && std::isfinite(cz);
}

// Given a diagram whose vertices are already filled (one per tet) and a validity
// flag per tet, build the dual edges (finite for interior faces, outward rays for
// hull faces) and the per-site cells. Shared by build() and build_power().
void finish(VoronoiDiagram& vor, const Mesh& mesh,
            const std::vector<char>& valid) {
    const int nt = static_cast<int>(mesh.tetrahedra.size());
    std::map<std::array<int, 3>, FaceRec> faces;
    for (int i = 0; i < nt; ++i) {
        const auto& t = mesh.tetrahedra[i];
        for (int k = 0; k < 4; ++k) {
            int f[3], j = 0;
            for (int m = 0; m < 4; ++m)
                if (m != k) f[j++] = t[m];
            auto key = sorted3(f[0], f[1], f[2]);
            auto it = faces.find(key);
            if (it == faces.end())
                faces[key] = FaceRec{{f[0], f[1], f[2]}, t[k], i, -1};
            else
                it->second.t1 = i;
        }
    }
    for (const auto& [key, rec] : faces) {
        if (rec.t1 >= 0) {
            if (valid[rec.t0] && valid[rec.t1])
                vor.edges.push_back({rec.t0, rec.t1, {}});
            continue;
        }
        if (!valid[rec.t0]) continue;
        const Point3 &a = mesh.points[rec.fv[0]], &b = mesh.points[rec.fv[1]],
                     &c = mesh.points[rec.fv[2]], &o = mesh.points[rec.opp];
        double e1[3] = {double(b.x) - a.x, double(b.y) - a.y, double(b.z) - a.z};
        double e2[3] = {double(c.x) - a.x, double(c.y) - a.y, double(c.z) - a.z};
        double n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2],
                       e1[0] * e2[1] - e1[1] * e2[0]};
        double to_o[3] = {double(o.x) - a.x, double(o.y) - a.y, double(o.z) - a.z};
        if (n[0] * to_o[0] + n[1] * to_o[1] + n[2] * to_o[2] > 0)
            for (double& v : n) v = -v;
        double len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (len <= 0) continue;
        vor.edges.push_back({rec.t0, -1,
                             {static_cast<Real>(n[0] / len),
                              static_cast<Real>(n[1] / len),
                              static_cast<Real>(n[2] / len)}});
    }
    vor.cells.assign(mesh.points.size(), {});
    for (int i = 0; i < nt; ++i)
        for (int v : mesh.tetrahedra[i]) vor.cells[v].push_back(i);
}

} // namespace

VoronoiDiagram build(const Mesh& delaunay) {
    VoronoiDiagram vor;
    const int nt = static_cast<int>(delaunay.tetrahedra.size());
    vor.vertices.resize(nt);
    std::vector<char> valid(nt, 1);
    for (int i = 0; i < nt; ++i) {
        const auto& t = delaunay.tetrahedra[i];
        double r = 0;
        if (!geom::circumcenter(delaunay.points[t[0]], delaunay.points[t[1]],
                                delaunay.points[t[2]], delaunay.points[t[3]],
                                vor.vertices[i], r)) {
            vor.vertices[i] = centroid(delaunay, t);
            valid[i] = 0;
        }
    }
    finish(vor, delaunay, valid);
    return vor;
}

VoronoiDiagram build_power(const Mesh& mesh, std::span<const double> weights) {
    VoronoiDiagram vor;
    const int nt = static_cast<int>(mesh.tetrahedra.size());
    vor.vertices.resize(nt);
    std::vector<char> valid(nt, 1);
    for (int i = 0; i < nt; ++i) {
        const auto& t = mesh.tetrahedra[i];
        Point3 p[4] = {mesh.points[t[0]], mesh.points[t[1]], mesh.points[t[2]],
                       mesh.points[t[3]]};
        double w[4];
        for (int k = 0; k < 4; ++k)
            w[k] = (static_cast<std::size_t>(t[k]) < weights.size()) ? weights[t[k]] : 0.0;
        if (!orthocenter(p, w, vor.vertices[i])) {
            vor.vertices[i] = centroid(mesh, t);
            valid[i] = 0;
        }
    }
    finish(vor, mesh, valid);
    return vor;
}


} // namespace cmg::voronoi
