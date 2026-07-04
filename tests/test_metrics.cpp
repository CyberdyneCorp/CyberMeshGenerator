// CyberMeshGenerator — tests for cmg::quality::report.
#include <array>
#include <cmath>
#include <random>
#include <vector>

#include "cmg/cmg.hpp"
#include "cmg/core/circumcenter.hpp"
#include "cmg/quality/metrics.hpp"
#include "harness.hpp"

namespace {

using cmg::Mesh;
using cmg::Point3;

double vec_len(const Point3& a, const Point3& b) {
    const double dx = double(a.x) - b.x, dy = double(a.y) - b.y,
                 dz = double(a.z) - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// |det[b-a,c-a,d-a]| / 6 — matches the module's volume convention.
double tet_vol(const Point3& a, const Point3& b, const Point3& c,
               const Point3& d) {
    const double bx = double(b.x) - a.x, by = double(b.y) - a.y, bz = double(b.z) - a.z;
    const double cx = double(c.x) - a.x, cy = double(c.y) - a.y, cz = double(c.z) - a.z;
    const double dx = double(d.x) - a.x, dy = double(d.y) - a.y, dz = double(d.z) - a.z;
    const double det = bx * (cy * dz - cz * dy) - by * (cx * dz - cz * dx) +
                       bz * (cx * dy - cy * dx);
    return std::fabs(det) / 6.0;
}

// Recompute radius-edge ratio the same way the module does.
double ratio_of(const Mesh& m, const cmg::Tetrahedron& t) {
    const std::array<Point3, 4> P = {m.points[t[0]], m.points[t[1]], m.points[t[2]],
                                     m.points[t[3]]};
    double se = 1e300;
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) se = std::min(se, vec_len(P[j], P[i]));
    Point3 cc;
    double radius = 0;
    if (!cmg::geom::circumcenter(P[0], P[1], P[2], P[3], cc, radius) || se <= 0)
        return -1.0;
    return radius / se;
}

} // namespace

CMG_TEST("report on random delaunay is self-consistent") {
    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> d(0.0, 1.0);
    std::vector<Point3> pts;
    for (int i = 0; i < 30; ++i)
        pts.push_back({cmg::Real(d(rng)), cmg::Real(d(rng)), cmg::Real(d(rng))});

    cmg::MeshOptions opts;
    auto res = cmg::delaunay(pts, opts);
    CMG_CHECK(res.has_value());
    const Mesh& mesh = res.value();
    CMG_CHECK(mesh.tet_count() > 0);

    const auto r = cmg::quality::report(mesh, 10);
    CMG_CHECK(r.num_tets == mesh.tet_count());

    int hist_sum = 0;
    for (int h : r.dihedral_histogram) hist_sum += h;
    CMG_CHECK(hist_sum == static_cast<int>(6 * mesh.tet_count()));

    double vol_sum = 0;
    for (const auto& t : mesh.tetrahedra)
        vol_sum += tet_vol(mesh.points[t[0]], mesh.points[t[1]], mesh.points[t[2]],
                           mesh.points[t[3]]);
    CMG_CHECK(std::fabs(r.total_volume - vol_sum) <= 1e-9 * (1.0 + vol_sum));

    CMG_CHECK(r.min_dihedral <= r.mean_dihedral + 1e-9);
    CMG_CHECK(r.mean_dihedral <= r.max_dihedral + 1e-9);
}

CMG_TEST("report on single regular-ish tet") {
    Mesh mesh;
    mesh.points = {{0, 0, 0},
                   {1, 0, 0},
                   {cmg::Real(0.5), cmg::Real(0.866), 0},
                   {cmg::Real(0.5), cmg::Real(0.289), cmg::Real(0.816)}};
    // Store with orient3d(v0,v1,v2,v3) < 0.
    cmg::Tetrahedron t = {0, 1, 2, 3};
    if (!(cmg::robust::orient3d(mesh.points[0], mesh.points[1], mesh.points[2],
                                mesh.points[3]) < 0.0))
        std::swap(t[2], t[3]);
    mesh.tetrahedra = {t};

    const auto r = cmg::quality::report(mesh, 10);
    CMG_CHECK(r.num_tets == 1);
    CMG_CHECK(r.min_dihedral >= 60.0 && r.min_dihedral <= 75.0);
    CMG_CHECK(r.min_radius_edge >= 0.5 && r.min_radius_edge <= 0.75);

    int hist_sum = 0;
    for (int h : r.dihedral_histogram) hist_sum += h;
    CMG_CHECK(hist_sum == 6);
}

CMG_TEST("worst_tets is in descending radius-edge order") {
    std::mt19937 rng(777);
    std::uniform_real_distribution<double> d(0.0, 1.0);
    std::vector<Point3> pts;
    for (int i = 0; i < 40; ++i)
        pts.push_back({cmg::Real(d(rng)), cmg::Real(d(rng)), cmg::Real(d(rng))});

    cmg::MeshOptions opts;
    auto res = cmg::delaunay(pts, opts);
    CMG_CHECK(res.has_value());
    const Mesh& mesh = res.value();

    const auto r = cmg::quality::report(mesh, 8);
    CMG_CHECK(!r.worst_tets.empty());
    CMG_CHECK(r.worst_tets.size() <= 8);

    double prev = 1e300;
    for (int idx : r.worst_tets) {
        CMG_CHECK(idx >= 0 && idx < static_cast<int>(mesh.tet_count()));
        const double re = ratio_of(mesh, mesh.tetrahedra[idx]);
        CMG_CHECK(re <= prev + 1e-9);
        prev = re;
    }
}
