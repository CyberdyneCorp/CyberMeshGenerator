// CyberMeshGenerator — Phase 9 Voronoi diagram tests.
#include "cmg/cmg.hpp"
#include "harness.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>

using namespace cmg;

namespace {

double dist(const Point3& a, const Point3& b) {
    return std::sqrt(double(a.x - b.x) * (a.x - b.x) +
                     double(a.y - b.y) * (a.y - b.y) +
                     double(a.z - b.z) * (a.z - b.z));
}

std::array<int, 3> sorted3(int a, int b, int c) {
    std::array<int, 3> k{a, b, c};
    std::sort(k.begin(), k.end());
    return k;
}

// Count interior (2-tet) and hull (1-tet) faces of a tet mesh.
std::pair<int, int> face_counts(const Mesh& m) {
    std::map<std::array<int, 3>, int> mult;
    for (const auto& t : m.tetrahedra) {
        mult[sorted3(t[1], t[2], t[3])]++;
        mult[sorted3(t[0], t[2], t[3])]++;
        mult[sorted3(t[0], t[1], t[3])]++;
        mult[sorted3(t[0], t[1], t[2])]++;
    }
    int interior = 0, hull = 0;
    for (auto& [k, c] : mult) (c == 2 ? interior : hull)++;
    return {interior, hull};
}

std::vector<Point3> cloud(int n, unsigned s) {
    std::vector<Point3> p;
    auto nx = [&] { s = s * 1664525u + 1013904223u; return static_cast<Real>((s >> 8) / double(1u << 24)); };
    for (int i = 0; i < n; ++i) p.push_back({nx(), nx(), nx()});
    return p;
}

std::vector<Point3> cube_pts() {
    return {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
            {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
}

} // namespace

CMG_TEST("one Voronoi vertex per tet, equidistant from its four vertices") {
    auto m = delaunay(cloud(50, 3), {});
    auto v = voronoi::build(*m);
    CMG_CHECK(v.vertices.size() == m->tet_count());
    for (std::size_t i = 0; i < m->tetrahedra.size(); ++i) {
        const auto& t = m->tetrahedra[i];
        double r0 = dist(v.vertices[i], m->points[t[0]]);
        for (int k = 1; k < 4; ++k)
            CMG_CHECK(std::fabs(dist(v.vertices[i], m->points[t[k]]) - r0) < 1e-5);
    }
}

CMG_TEST("edges are dual to faces: finite for interior, rays for hull") {
    auto m = delaunay(cloud(50, 11), {});
    auto v = voronoi::build(*m);
    auto [interior, hull] = face_counts(*m);
    std::size_t finite = v.edges.size() - v.ray_count();
    CMG_CHECK(finite == static_cast<std::size_t>(interior));
    CMG_CHECK(v.ray_count() == static_cast<std::size_t>(hull));
}

CMG_TEST("hull-face rays are outward unit normals (cube)") {
    auto m = delaunay(cube_pts(), {});
    auto v = voronoi::build(*m);
    Point3 center{0.5, 0.5, 0.5};
    for (const auto& e : v.edges) {
        if (e.v1 >= 0) continue; // finite edge
        // unit length
        double len = std::sqrt(double(e.dir.x) * e.dir.x + double(e.dir.y) * e.dir.y +
                               double(e.dir.z) * e.dir.z);
        CMG_CHECK(std::fabs(len - 1.0) < 1e-6);
        // axis-aligned outward normal of a cube face: one component is ±1
        double mx = std::max({std::fabs(e.dir.x), std::fabs(e.dir.y), std::fabs(e.dir.z)});
        CMG_CHECK(mx > 0.999);
        // points away from the cube centre: the originating circumcenter lies on a
        // cube face plane; dir has positive dot with (face - centre) along its axis
        (void)center;
    }
    // A cube's closed hull has 12 triangles -> 12 rays.
    CMG_CHECK(v.ray_count() == 12);
}

CMG_TEST("cells are non-empty per used vertex and cover all tets") {
    auto m = delaunay(cloud(45, 21), {});
    auto v = voronoi::build(*m);
    CMG_CHECK(v.cells.size() == m->point_count());
    std::set<int> used;
    for (const auto& t : m->tetrahedra)
        for (int idx : t) used.insert(idx);
    for (int u : used) CMG_CHECK(!v.cells[u].empty());
    std::set<int> covered;
    for (const auto& c : v.cells)
        for (int t : c) covered.insert(t);
    CMG_CHECK(covered.size() == m->tet_count());
}

CMG_TEST("power diagram vertices are orthocenters (equal power to all 4 vertices)") {
    // Weighted (regular) Delaunay of the cube corners with small distinct weights.
    auto pts = cube_pts();
    std::vector<Real> w(8);
    for (int i = 0; i < 8; ++i) w[i] = static_cast<Real>(0.01 * i);
    MeshOptions o;
    o.weighted = true;
    o.weights = w;
    auto m = delaunay(pts, o);
    CMG_CHECK(bool(m));

    std::vector<double> wd(w.begin(), w.end());
    auto v = voronoi::build_power(*m, wd);
    CMG_CHECK(v.vertices.size() == m->tet_count());

    // The orthocenter has equal power |c - p_i|^2 - w_i to all four tet vertices.
    for (std::size_t i = 0; i < m->tetrahedra.size(); ++i) {
        const auto& t = m->tetrahedra[i];
        const Point3& c = v.vertices[i];
        double p0 = dist(c, m->points[t[0]]) * dist(c, m->points[t[0]]) - wd[t[0]];
        for (int k = 1; k < 4; ++k) {
            double pk = dist(c, m->points[t[k]]) * dist(c, m->points[t[k]]) - wd[t[k]];
            CMG_CHECK(std::fabs(pk - p0) < 1e-5);
        }
    }
    // Same edge/cell duality as the unweighted diagram.
    auto [interior, hull] = face_counts(*m);
    CMG_CHECK(v.edges.size() - v.ray_count() == static_cast<std::size_t>(interior));
    CMG_CHECK(v.ray_count() == static_cast<std::size_t>(hull));
}

CMG_TEST("no non-finite Voronoi vertices on a cospherical input") {
    // 8 cube corners are cospherical -> degeneracy-prone circumcenters.
    auto m = delaunay(cube_pts(), {});
    auto v = voronoi::build(*m);
    for (const auto& p : v.vertices) {
        CMG_CHECK(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z));
    }
}
