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

} // namespace

VoronoiDiagram build(const Mesh& delaunay) {
    VoronoiDiagram vor;
    const int nt = static_cast<int>(delaunay.tetrahedra.size());

    // 1. Vertices: one circumcenter per tetrahedron (centroid fallback if
    // degenerate, kept finite; degenerate tets are excluded from dual edges).
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

    // 2. Faces -> edges. Build face adjacency, keyed on sorted vertex triples.
    std::map<std::array<int, 3>, FaceRec> faces;
    for (int i = 0; i < nt; ++i) {
        const auto& t = delaunay.tetrahedra[i];
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
        if (rec.t1 >= 0) { // interior face -> finite edge
            if (valid[rec.t0] && valid[rec.t1])
                vor.edges.push_back({rec.t0, rec.t1, {}});
            continue;
        }
        if (!valid[rec.t0]) continue; // hull face of a degenerate tet -> skip
        // Convex-hull face -> ray. Outward normal points away from the opposite vertex.
        const Point3 &a = delaunay.points[rec.fv[0]], &b = delaunay.points[rec.fv[1]],
                     &c = delaunay.points[rec.fv[2]], &o = delaunay.points[rec.opp];
        double e1[3] = {double(b.x) - a.x, double(b.y) - a.y, double(b.z) - a.z};
        double e2[3] = {double(c.x) - a.x, double(c.y) - a.y, double(c.z) - a.z};
        double n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2],
                       e1[0] * e2[1] - e1[1] * e2[0]};
        double to_o[3] = {double(o.x) - a.x, double(o.y) - a.y, double(o.z) - a.z};
        if (n[0] * to_o[0] + n[1] * to_o[1] + n[2] * to_o[2] > 0)
            for (double& v : n) v = -v; // flip to point away from the opposite vertex
        double len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (len <= 0) continue;
        vor.edges.push_back(
            {rec.t0, -1,
             {static_cast<Real>(n[0] / len), static_cast<Real>(n[1] / len),
              static_cast<Real>(n[2] / len)}});
    }

    // 3. Cells: per input vertex, the incident tetrahedra (their circumcenters).
    vor.cells.assign(delaunay.points.size(), {});
    for (int i = 0; i < nt; ++i)
        for (int v : delaunay.tetrahedra[i])
            vor.cells[v].push_back(i);

    return vor;
}

} // namespace cmg::voronoi
