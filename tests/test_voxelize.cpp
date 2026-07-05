// CyberMeshGenerator — solid voxelization (occupancy grid + signed-distance field).
#include "cmg/cmg.hpp"
#include "cmg/voxelize/voxelize.hpp"
#include "harness.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

using namespace cmg;

namespace {

// A unit cube whose faces are subdivided into an n x n grid of quads (two triangles
// each), with shared edge/corner vertices. Watertight, convex, exact volume 1 — the
// same helper used by test_simplify.cpp's carve stress test.
PLC subdivided_cube(int n) {
    PLC p;
    std::map<std::array<int, 3>, int> vid;
    auto vert = [&](int x, int y, int z) {
        std::array<int, 3> k{x, y, z};
        auto it = vid.find(k);
        if (it != vid.end()) return it->second;
        int id = static_cast<int>(p.points.size());
        p.points.push_back({static_cast<Real>(double(x) / n),
                            static_cast<Real>(double(y) / n),
                            static_cast<Real>(double(z) / n)});
        vid[k] = id;
        return id;
    };
    auto quad = [&](int a, int b, int c, int d) {
        Facet f;
        f.polygons.push_back(Polygon{{a, b, c}});
        p.facets.push_back(std::move(f));
        Facet g;
        g.polygons.push_back(Polygon{{a, c, d}});
        p.facets.push_back(std::move(g));
    };
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            quad(vert(i, j, 0), vert(i + 1, j, 0), vert(i + 1, j + 1, 0), vert(i, j + 1, 0));
            quad(vert(i, j, n), vert(i, j + 1, n), vert(i + 1, j + 1, n), vert(i + 1, j, n));
            quad(vert(i, 0, j), vert(i, 0, j + 1), vert(i + 1, 0, j + 1), vert(i + 1, 0, j));
            quad(vert(i, n, j), vert(i + 1, n, j), vert(i + 1, n, j + 1), vert(i, n, j + 1));
            quad(vert(0, i, j), vert(0, i + 1, j), vert(0, i + 1, j + 1), vert(0, i, j + 1));
            quad(vert(n, i, j), vert(n, i, j + 1), vert(n, i + 1, j + 1), vert(n, i + 1, j));
        }
    return p;
}

// A triangulated UV-sphere PLC (radius r), watertight and rounded — a genuine partial-
// cell test for occupancy convergence.
PLC sphere_plc(double r, int nlat, int nlon) {
    PLC p;
    std::vector<std::vector<int>> id(nlat + 1, std::vector<int>(nlon));
    int top = 0, bot = 0;
    p.points.push_back({0, 0, static_cast<Real>(r)});
    for (int i = 1; i < nlat; ++i) {
        double th = M_PI * i / nlat;
        for (int j = 0; j < nlon; ++j) {
            double ph = 2 * M_PI * j / nlon;
            id[i][j] = static_cast<int>(p.points.size());
            p.points.push_back({static_cast<Real>(r * std::sin(th) * std::cos(ph)),
                                static_cast<Real>(r * std::sin(th) * std::sin(ph)),
                                static_cast<Real>(r * std::cos(th))});
        }
    }
    bot = static_cast<int>(p.points.size());
    p.points.push_back({0, 0, static_cast<Real>(-r)});
    auto tri = [&](int a, int b, int c) {
        Facet f;
        f.polygons.push_back(Polygon{{a, b, c}});
        p.facets.push_back(std::move(f));
    };
    for (int j = 0; j < nlon; ++j)
        tri(top, id[1][j], id[1][(j + 1) % nlon]);
    for (int i = 1; i < nlat - 1; ++i)
        for (int j = 0; j < nlon; ++j) {
            int j1 = (j + 1) % nlon;
            tri(id[i][j], id[i + 1][j], id[i + 1][j1]);
            tri(id[i][j], id[i + 1][j1], id[i][j1]);
        }
    for (int j = 0; j < nlon; ++j)
        tri(id[nlat - 1][j], bot, id[nlat - 1][(j + 1) % nlon]);
    return p;
}

// Occupied-cell volume of an Occupancy grid.
double occupied_volume(const voxelize::VoxelGrid& g) {
    std::size_t n = 0;
    for (std::uint8_t c : g.occupancy) n += c;
    return n * g.spacing * g.spacing * g.spacing;
}

} // namespace

CMG_TEST("voxelize occupancy approximates a unit cube's volume and converges") {
    PLC cube = subdivided_cube(4);
    // Single precision loses a little on the grid of coplanar face vertices; double is
    // near exact. Both must land within tolerance and the finer grid no worse.
    const double tol = sizeof(Real) == 4 ? 0.10 : 0.03;

    auto coarse = voxelize::voxelize(cube, {.resolution = 12});
    auto fine = voxelize::voxelize(cube, {.resolution = 48});
    CMG_CHECK(bool(coarse) && bool(fine));
    double ec = std::fabs(occupied_volume(*coarse) - 1.0);
    double ef = std::fabs(occupied_volume(*fine) - 1.0);
    CMG_CHECK(ec < tol);
    CMG_CHECK(ef < tol);
    CMG_CHECK(ef <= ec + 1e-9); // finer never worse
}

CMG_TEST("voxelize occupancy converges toward a sphere's analytic volume") {
    PLC s = sphere_plc(1.0, 24, 24);
    double exact = 4.0 / 3.0 * M_PI; // r = 1
    auto coarse = voxelize::voxelize(s, {.resolution = 16});
    auto fine = voxelize::voxelize(s, {.resolution = 40});
    CMG_CHECK(bool(coarse) && bool(fine));
    double ec = std::fabs(occupied_volume(*coarse) - exact) / exact;
    double ef = std::fabs(occupied_volume(*fine) - exact) / exact;
    CMG_CHECK(ef < 0.10);       // fine grid tracks the tessellated volume
    CMG_CHECK(ef <= ec + 0.02); // and does not get worse with resolution
}

CMG_TEST("voxelize is deterministic for fixed options") {
    PLC cube = subdivided_cube(4);
    voxelize::VoxelOptions o{.resolution = 20};
    auto a = voxelize::voxelize(cube, o);
    auto b = voxelize::voxelize(cube, o);
    CMG_CHECK(bool(a) && bool(b));
    CMG_CHECK(a->nx == b->nx && a->ny == b->ny && a->nz == b->nz);
    CMG_CHECK(a->spacing == b->spacing);
    CMG_CHECK(a->origin == b->origin);
    CMG_CHECK(a->occupancy == b->occupancy);
}

CMG_TEST("voxelize classifies a watertight cube without boundary speckle") {
    // With pad=1 the R=16 cube occupies a solid interior block [1..16] on every axis and
    // nothing outside it — no misclassified boundary cells.
    const int R = 16, pad = 1;
    PLC cube = subdivided_cube(4);
    auto g = voxelize::voxelize(cube, {.resolution = R, .pad = pad});
    CMG_CHECK(bool(g));
    CMG_CHECK(g->nx == R + 2 * pad && g->ny == R + 2 * pad && g->nz == R + 2 * pad);

    std::size_t occupied = 0, stray = 0, missing = 0;
    for (int k = 0; k < g->nz; ++k)
        for (int j = 0; j < g->ny; ++j)
            for (int i = 0; i < g->nx; ++i) {
                bool inside_block = i >= pad && i < pad + R && j >= pad && j < pad + R &&
                                    k >= pad && k < pad + R;
                std::uint8_t v = g->occupancy[g->index(i, j, k)];
                if (v) ++occupied;
                if (v && !inside_block) ++stray;   // speckle outside the solid
                if (!v && inside_block) ++missing; // hole inside the solid
            }
    CMG_CHECK(stray == 0);
    CMG_CHECK(missing == 0);
    CMG_CHECK(occupied == static_cast<std::size_t>(R) * R * R);
}

CMG_TEST("voxelize signed distance is negative inside and positive outside") {
    const int R = 16, pad = 2;
    PLC cube = subdivided_cube(4);
    auto g = voxelize::voxelize(cube, {.resolution = R, .pad = pad,
                                       .mode = voxelize::VoxelMode::SignedDistance});
    CMG_CHECK(bool(g));
    CMG_CHECK(g->distance.size() == g->cell_count());

    // Cell nearest the cube center: deep inside -> negative, magnitude ~0.5 (half side).
    int mid = pad + R / 2;
    float center = g->distance[g->index(mid, mid, mid)];
    CMG_CHECK(center < 0.0f);
    CMG_CHECK(std::fabs(center) > 0.3f && std::fabs(center) < 0.6f);

    // A corner cell of the padded region is outside -> positive.
    float corner = g->distance[g->index(0, 0, 0)];
    CMG_CHECK(corner > 0.0f);
}

CMG_TEST("voxelize rejects invalid input") {
    PLC cube = subdivided_cube(2);
    CMG_CHECK(!voxelize::voxelize(cube, {.resolution = 0})); // resolution < 1
    PLC empty;
    CMG_CHECK(!voxelize::voxelize(empty, {.resolution = 8})); // no points
}

namespace {

// Squared distance from p to triangle abc (independent brute-force reference).
double ref_pt_tri_d2(const double p[3], const double a[3], const double b[3],
                     const double c[3]) {
    auto dot = [](const double u[3], const double v[3]) {
        return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
    };
    double ab[3] = {b[0]-a[0], b[1]-a[1], b[2]-a[2]};
    double ac[3] = {c[0]-a[0], c[1]-a[1], c[2]-a[2]};
    double ap[3] = {p[0]-a[0], p[1]-a[1], p[2]-a[2]};
    auto d2q = [&](const double q[3]) {
        double t[3] = {p[0]-q[0], p[1]-q[1], p[2]-q[2]};
        return dot(t, t);
    };
    double d1 = dot(ab, ap), d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return d2q(a);
    double bp[3] = {p[0]-b[0], p[1]-b[1], p[2]-b[2]};
    double d3 = dot(ab, bp), d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return d2q(b);
    double vc = d1*d4 - d3*d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        double v = d1 / (d1 - d3), q[3] = {a[0]+v*ab[0], a[1]+v*ab[1], a[2]+v*ab[2]};
        return d2q(q);
    }
    double cp[3] = {p[0]-c[0], p[1]-c[1], p[2]-c[2]};
    double d5 = dot(ab, cp), d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return d2q(c);
    double vb = d5*d2 - d1*d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        double w = d2 / (d2 - d6), q[3] = {a[0]+w*ac[0], a[1]+w*ac[1], a[2]+w*ac[2]};
        return d2q(q);
    }
    double va = d3*d6 - d5*d4;
    if (va <= 0 && (d4-d3) >= 0 && (d5-d6) >= 0) {
        double w = (d4-d3) / ((d4-d3)+(d5-d6));
        double q[3] = {b[0]+w*(c[0]-b[0]), b[1]+w*(c[1]-b[1]), b[2]+w*(c[2]-b[2])};
        return d2q(q);
    }
    double den = 1.0 / (va+vb+vc), v = vb*den, w = vc*den;
    double q[3] = {a[0]+ab[0]*v+ac[0]*w, a[1]+ab[1]*v+ac[1]*w, a[2]+ab[2]*v+ac[2]*w};
    return d2q(q);
}

std::vector<std::array<int, 3>> plc_triangles(const PLC& p) {
    std::vector<std::array<int, 3>> t;
    for (const Facet& f : p.facets)
        for (const Polygon& poly : f.polygons)
            for (std::size_t i = 2; i < poly.vertices.size(); ++i)
                t.push_back({poly.vertices[0], poly.vertices[i-1], poly.vertices[i]});
    return t;
}

} // namespace

CMG_TEST("accelerated signed distance equals a brute-force reference") {
    PLC s = sphere_plc(1.0, 16, 16); // ~450 triangles — exercises the ring search
    const int R = 20;
    auto g = voxelize::voxelize(s, {.resolution = R, .pad = 1,
                                    .mode = voxelize::VoxelMode::SignedDistance});
    CMG_CHECK(bool(g));

    const auto tris = plc_triangles(s);
    double worst = 0;
    for (int k = 0; k < g->nz; ++k)
        for (int j = 0; j < g->ny; ++j)
            for (int i = 0; i < g->nx; ++i) {
                double p[3] = {double(g->origin.x) + i * g->spacing,
                               double(g->origin.y) + j * g->spacing,
                               double(g->origin.z) + k * g->spacing};
                double best = std::numeric_limits<double>::max();
                for (const auto& t : tris) {
                    double a[3] = {double(s.points[t[0]].x), double(s.points[t[0]].y),
                                   double(s.points[t[0]].z)};
                    double b[3] = {double(s.points[t[1]].x), double(s.points[t[1]].y),
                                   double(s.points[t[1]].z)};
                    double c[3] = {double(s.points[t[2]].x), double(s.points[t[2]].y),
                                   double(s.points[t[2]].z)};
                    best = std::min(best, ref_pt_tri_d2(p, a, b, c));
                }
                double got = std::fabs(g->distance[g->index(i, j, k)]);
                worst = std::max(worst, std::fabs(std::sqrt(best) - got));
            }
    CMG_CHECK(worst < 1e-5); // the spatial index finds the true nearest triangle
}
