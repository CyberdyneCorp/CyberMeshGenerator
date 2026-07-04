// CyberMeshGenerator — surface simplification + accelerated-carve tests.
#include "cmg/cmg.hpp"
#include "cmg/simplify/simplify.hpp"
#include "harness.hpp"

#include <array>
#include <cmath>
#include <map>
#include <set>
#include <vector>

using namespace cmg;

namespace {

// A triangulated UV-sphere PLC (radius r, `nlat` x `nlon` grid). Dense enough to
// exercise the simplifier and the spatially-indexed carve.
PLC sphere_plc(double r, int nlat, int nlon) {
    PLC p;
    std::vector<std::vector<int>> id(nlat + 1, std::vector<int>(nlon));
    int top = 0, bot = 0;
    p.points.push_back({0, 0, static_cast<Real>(r)});
    top = 0;
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
    for (int j = 0; j < nlon; ++j) // top cap
        tri(top, id[1][j], id[1][(j + 1) % nlon]);
    for (int i = 1; i < nlat - 1; ++i) // quads
        for (int j = 0; j < nlon; ++j) {
            int j1 = (j + 1) % nlon;
            tri(id[i][j], id[i + 1][j], id[i + 1][j1]);
            tri(id[i][j], id[i + 1][j1], id[i][j1]);
        }
    for (int j = 0; j < nlon; ++j) // bottom cap
        tri(id[nlat - 1][j], bot, id[nlat - 1][(j + 1) % nlon]);
    return p;
}

// A unit cube whose faces are subdivided into an n x n grid of quads (each split into
// two triangles), with shared edge/corner vertices. Convex, non-degenerate, and its
// exact volume is 1 — a clean stress test for the spatially-indexed carve.
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

int triangle_count(const PLC& p) {
    int n = 0;
    for (const Facet& f : p.facets)
        for (const Polygon& poly : f.polygons)
            n += static_cast<int>(poly.vertices.size()) - 2;
    return n;
}

// Enclosed volume of a triangulated closed surface (divergence theorem).
double enclosed_volume(const PLC& p) {
    double v = 0;
    for (const Facet& f : p.facets)
        for (const Polygon& poly : f.polygons) {
            const auto& vtx = poly.vertices;
            for (std::size_t i = 2; i < vtx.size(); ++i) {
                const Point3& a = p.points[vtx[0]];
                const Point3& b = p.points[vtx[i - 1]];
                const Point3& c = p.points[vtx[i]];
                v += (double(a.x) * (double(b.y) * c.z - double(b.z) * c.y) -
                      double(a.y) * (double(b.x) * c.z - double(b.z) * c.x) +
                      double(a.z) * (double(b.x) * c.y - double(b.y) * c.x)) /
                     6.0;
            }
        }
    return std::fabs(v);
}

double mesh_volume(const Mesh& m) {
    double v = 0;
    for (const auto& t : m.tetrahedra) {
        const Point3& a = m.points[t[0]];
        const Point3& b = m.points[t[1]];
        const Point3& c = m.points[t[2]];
        const Point3& d = m.points[t[3]];
        double e1[3] = {double(b.x) - a.x, double(b.y) - a.y, double(b.z) - a.z};
        double e2[3] = {double(c.x) - a.x, double(c.y) - a.y, double(c.z) - a.z};
        double e3[3] = {double(d.x) - a.x, double(d.y) - a.y, double(d.z) - a.z};
        double cr[3] = {e2[1] * e3[2] - e2[2] * e3[1], e2[2] * e3[0] - e2[0] * e3[2],
                        e2[0] * e3[1] - e2[1] * e3[0]};
        v += std::fabs(e1[0] * cr[0] + e1[1] * cr[1] + e1[2] * cr[2]) / 6.0;
    }
    return v;
}

} // namespace

CMG_TEST("simplify reduces triangle count and preserves enclosed volume") {
    PLC s = sphere_plc(1.0, 24, 24);
    int before = triangle_count(s);
    double v0 = enclosed_volume(s);

    auto r = simplify::simplify(s, {.grid = 8});
    CMG_CHECK(bool(r));
    int after = triangle_count(*r);
    CMG_CHECK(after < before);          // genuinely coarser
    CMG_CHECK(after > 0);
    double v1 = enclosed_volume(*r);
    CMG_CHECK(std::fabs(v1 - v0) / v0 < 0.15); // shape preserved within a few %
}

CMG_TEST("simplify is deterministic for a fixed grid") {
    PLC s = sphere_plc(1.0, 20, 20);
    auto a = simplify::simplify(s, {.grid = 10});
    auto b = simplify::simplify(s, {.grid = 10});
    CMG_CHECK(bool(a) && bool(b));
    CMG_CHECK(a->points.size() == b->points.size());
    CMG_CHECK(triangle_count(*a) == triangle_count(*b));
}

CMG_TEST("coarser grid yields no more triangles than a finer grid") {
    PLC s = sphere_plc(1.0, 28, 28);
    auto coarse = simplify::simplify(s, {.grid = 8});
    auto fine = simplify::simplify(s, {.grid = 24});
    CMG_CHECK(bool(coarse) && bool(fine));
    CMG_CHECK(triangle_count(*coarse) <= triangle_count(*fine));
}

CMG_TEST("simplify rejects invalid input") {
    PLC empty;
    CMG_CHECK(!simplify::simplify(empty, {.grid = 8}));  // no points
    PLC s = sphere_plc(1.0, 8, 8);
    CMG_CHECK(!simplify::simplify(s, {.grid = 0}));       // grid < 1
}

CMG_TEST("accelerated carve meshes a dense closed surface with conserved volume") {
    // A subdivided cube = 6 * 8 * 8 * 2 = 768 boundary triangles: enough to build and
    // exercise the per-direction spatial index. The grid classification tests a
    // superset of the brute-force hits, so the carve conserves the exact volume (1).
    PLC s = subdivided_cube(8);
    MeshOptions o;
    o.plc = true;
    auto r = tetrahedralize(s, o);
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_count() > 0);
    double vm = mesh_volume(*r);
    double vs = enclosed_volume(s); // == 1 for the unit cube
    // Double precision is exact here (~1e-14); single precision loses a little on the
    // grid of coplanar face vertices (ray-cast classification is less robust in float).
    const double tol = sizeof(Real) == 4 ? 0.05 : 1e-6;
    CMG_CHECK(std::fabs(vm - vs) / vs < tol);
}
