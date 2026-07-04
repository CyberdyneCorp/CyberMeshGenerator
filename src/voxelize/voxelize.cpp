// CyberMeshGenerator — solid voxelization via exact vertical ray-parity.
//
// Fan-triangulates the PLC boundary, buckets the triangles by their xy-projection (a
// uniform 2-D grid mirroring the CarveGrid idea in tetrahedralize_plc.cpp), then for
// each grid column shoots the vertical line through the cell centers and collects its
// crossings with the bucketed triangles. Both the plane-side test and the in-triangle
// test are exact orient3d evaluations, so a watertight mesh classifies without the
// epsilon-ray speckle the carve tolerates. A cell is inside iff an odd number of
// crossings lie above its center. SignedDistance mode reuses that inside/outside bit
// for the sign and computes the exact point-to-triangle distance in double.
#include "cmg/voxelize/voxelize.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "cmg/predicates/robust.hpp"

namespace cmg::voxelize {
namespace {

using Tri = std::array<int, 3>;

// Fan-triangulate every facet polygon into triangles over the PLC vertices.
std::vector<Tri> boundary_triangles(const PLC& plc) {
    std::vector<Tri> tris;
    for (const Facet& f : plc.facets)
        for (const Polygon& poly : f.polygons) {
            const auto& v = poly.vertices;
            for (std::size_t i = 2; i < v.size(); ++i)
                tris.push_back({v[0], v[i - 1], v[i]});
        }
    return tris;
}

int sign_of(double v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

// Axis-aligned grid geometry. `corner` is the world coord of the MIN corner of cell
// (0,0,0); the cell center is corner + (i+0.5)*spacing per axis.
struct Grid {
    double corner[3] = {0, 0, 0};
    double spacing = 0;
    int nx = 0, ny = 0, nz = 0;
};

// Grid over the point bounding box: `resolution` cubic cells along the longest axis,
// the core region centered on the bbox, then `pad` margin cells added on every side.
Grid make_grid(const std::vector<Point3>& pts, const VoxelOptions& opts) {
    Point3 lo = pts[0], hi = pts[0];
    for (const Point3& p : pts)
        for (int a = 0; a < 3; ++a) {
            if (p[a] < lo[a]) lo[a] = p[a];
            if (p[a] > hi[a]) hi[a] = p[a];
        }
    double ext = 0;
    for (int a = 0; a < 3; ++a) ext = std::max(ext, double(hi[a]) - double(lo[a]));

    Grid g;
    g.spacing = ext / opts.resolution;
    if (!(g.spacing > 0)) return g; // caller rejects a zero-extent PLC
    int* dims[3] = {&g.nx, &g.ny, &g.nz};
    for (int a = 0; a < 3; ++a) {
        double e = double(hi[a]) - double(lo[a]);
        int core = std::max(1, static_cast<int>(std::ceil(e / g.spacing)));
        double covered = core * g.spacing;
        g.corner[a] = double(lo[a]) - (covered - e) / 2.0 - opts.pad * g.spacing;
        *dims[a] = core + 2 * opts.pad;
    }
    return g;
}

// One 2-D bucket grid over the xy-plane, indexed j*nx + i. A triangle is added to every
// column its xy bounding box overlaps, so a column only tests nearby triangles.
std::vector<std::vector<int>> build_xy_buckets(const Grid& g,
                                               const std::vector<Point3>& pts,
                                               const std::vector<Tri>& tris) {
    std::vector<std::vector<int>> cells(static_cast<std::size_t>(g.nx) * g.ny);
    auto cx = [&](double x) {
        int i = static_cast<int>(std::floor((x - g.corner[0]) / g.spacing));
        return i < 0 ? 0 : (i >= g.nx ? g.nx - 1 : i);
    };
    auto cy = [&](double y) {
        int j = static_cast<int>(std::floor((y - g.corner[1]) / g.spacing));
        return j < 0 ? 0 : (j >= g.ny ? g.ny - 1 : j);
    };
    for (int t = 0; t < static_cast<int>(tris.size()); ++t) {
        int i0 = g.nx, i1 = 0, j0 = g.ny, j1 = 0;
        for (int v = 0; v < 3; ++v) {
            const Point3& p = pts[tris[t][v]];
            int i = cx(p.x), j = cy(p.y);
            i0 = std::min(i0, i); i1 = std::max(i1, i);
            j0 = std::min(j0, j); j1 = std::max(j1, j);
        }
        for (int j = j0; j <= j1; ++j)
            for (int i = i0; i <= i1; ++i)
                cells[static_cast<std::size_t>(j) * g.nx + i].push_back(t);
    }
    return cells;
}

// z where the vertical line (x,y) meets the plane of triangle abc. Only called when a
// crossing was proven, which guarantees the plane is non-vertical (nz != 0).
double plane_z(const Point3& a, const Point3& b, const Point3& c, double x, double y) {
    double ux = double(b.x) - a.x, uy = double(b.y) - a.y, uz = double(b.z) - a.z;
    double vx = double(c.x) - a.x, vy = double(c.y) - a.y, vz = double(c.z) - a.z;
    double nx = uy * vz - uz * vy;
    double ny = uz * vx - ux * vz;
    double nz = ux * vy - uy * vx;
    return double(a.z) - (nx * (x - double(a.x)) + ny * (y - double(a.y))) / nz;
}

// Exact crossings of the vertical segment P=(x,y,zmin)..Q=(x,y,zmax) with the bucketed
// triangles. A triangle is crossed iff P and Q lie on opposite sides of its plane AND
// the line PQ passes through the triangle (the three edge orientations agree). Returns
// false if any orient3d is exactly 0 (the line grazes an edge/vertex) so the caller can
// nudge and retry; otherwise appends each crossing's z-height to `heights`.
bool column_crossings(double x, double y, double zmin, double zmax,
                      const std::vector<Point3>& pts, const std::vector<Tri>& tris,
                      const std::vector<int>& bucket, std::vector<double>& heights) {
    heights.clear();
    const Point3 P{static_cast<Real>(x), static_cast<Real>(y), static_cast<Real>(zmin)};
    const Point3 Q{static_cast<Real>(x), static_cast<Real>(y), static_cast<Real>(zmax)};
    for (int ti : bucket) {
        const Point3& a = pts[tris[ti][0]];
        const Point3& b = pts[tris[ti][1]];
        const Point3& c = pts[tris[ti][2]];
        int sa = sign_of(robust::orient3d(P, a, b, c));
        int sb = sign_of(robust::orient3d(Q, a, b, c));
        if (sa == 0 || sb == 0) return false; // grazes the triangle's plane
        if (sa == sb) continue;               // P,Q on the same side: no crossing
        int s1 = sign_of(robust::orient3d(P, Q, a, b));
        int s2 = sign_of(robust::orient3d(P, Q, b, c));
        int s3 = sign_of(robust::orient3d(P, Q, c, a));
        if (s1 == 0 || s2 == 0 || s3 == 0) return false; // grazes an edge/vertex
        if (s1 != s2 || s2 != s3) continue;              // line misses the triangle
        heights.push_back(plane_z(a, b, c, x, y));
    }
    return true;
}

// Deterministic tiny (x,y) nudges, in units of spacing, used when a column grazes a
// shared edge/vertex (an exact orient3d == 0). They stay well within the column's cell,
// so the same triangle bucket still applies.
constexpr double kNudge[][2] = {
    {1.0e-6, 3.0e-7},  {-7.0e-7, 5.0e-7}, {4.0e-7, -9.0e-7},
    {-2.0e-6, -1.0e-6}, {8.0e-7, 6.0e-7}, {-5.0e-7, -4.0e-6},
};

// Crossings for a column, nudging the (x,y) sample off any grazing configuration.
void solve_column(double x, double y, double zmin, double zmax,
                  const std::vector<Point3>& pts, const std::vector<Tri>& tris,
                  const std::vector<int>& bucket, double spacing,
                  std::vector<double>& heights) {
    if (column_crossings(x, y, zmin, zmax, pts, tris, bucket, heights)) return;
    for (const auto& d : kNudge)
        if (column_crossings(x + d[0] * spacing, y + d[1] * spacing, zmin, zmax, pts,
                             tris, bucket, heights))
            return;
    // All nudges grazed (astronomically unlikely for a real mesh): keep the last
    // partial result rather than failing the whole grid.
}

// Inside/outside byte per cell by vertical ray parity: cell (i,j,k) is inside iff an odd
// number of the column's crossings lie above its center z.
std::vector<std::uint8_t> classify_occupancy(const Grid& g,
                                             const std::vector<Point3>& pts,
                                             const std::vector<Tri>& tris,
                                             const std::vector<std::vector<int>>& buckets) {
    const std::size_t total = static_cast<std::size_t>(g.nx) * g.ny * g.nz;
    std::vector<std::uint8_t> occ(total, 0);
    const double zmin = g.corner[2] - g.spacing;
    const double zmax = g.corner[2] + (g.nz + 1) * g.spacing;
    std::vector<double> heights;
    for (int j = 0; j < g.ny; ++j)
        for (int i = 0; i < g.nx; ++i) {
            double cx = g.corner[0] + (i + 0.5) * g.spacing;
            double cy = g.corner[1] + (j + 0.5) * g.spacing;
            solve_column(cx, cy, zmin, zmax, pts, tris,
                         buckets[static_cast<std::size_t>(j) * g.nx + i], g.spacing,
                         heights);
            std::sort(heights.begin(), heights.end());
            for (int k = 0; k < g.nz; ++k) {
                double zc = g.corner[2] + (k + 0.5) * g.spacing;
                std::size_t above =
                    heights.end() - std::upper_bound(heights.begin(), heights.end(), zc);
                if (above & 1u)
                    occ[(static_cast<std::size_t>(k) * g.ny + j) * g.nx + i] = 1;
            }
        }
    return occ;
}

double dot3(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Squared distance from p to triangle abc (Ericson, Real-Time Collision Detection).
double point_tri_dist2(const double p[3], const double a[3], const double b[3],
                       const double c[3]) {
    double ab[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    double ac[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    double ap[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
    double d1 = dot3(ab, ap), d2 = dot3(ac, ap);
    auto dist2 = [&](const double q[3]) {
        double dx = p[0] - q[0], dy = p[1] - q[1], dz = p[2] - q[2];
        return dx * dx + dy * dy + dz * dz;
    };
    if (d1 <= 0 && d2 <= 0) return dist2(a); // region: vertex a

    double bp[3] = {p[0] - b[0], p[1] - b[1], p[2] - b[2]};
    double d3 = dot3(ab, bp), d4 = dot3(ac, bp);
    if (d3 >= 0 && d4 <= d3) return dist2(b); // region: vertex b

    double vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) { // region: edge ab
        double v = d1 / (d1 - d3);
        double q[3] = {a[0] + v * ab[0], a[1] + v * ab[1], a[2] + v * ab[2]};
        return dist2(q);
    }
    double cp[3] = {p[0] - c[0], p[1] - c[1], p[2] - c[2]};
    double d5 = dot3(ab, cp), d6 = dot3(ac, cp);
    if (d6 >= 0 && d5 <= d6) return dist2(c); // region: vertex c

    double vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) { // region: edge ac
        double w = d2 / (d2 - d6);
        double q[3] = {a[0] + w * ac[0], a[1] + w * ac[1], a[2] + w * ac[2]};
        return dist2(q);
    }
    double va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) { // region: edge bc
        double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        double q[3] = {b[0] + w * (c[0] - b[0]), b[1] + w * (c[1] - b[1]),
                       b[2] + w * (c[2] - b[2])};
        return dist2(q);
    }
    // interior: project onto the plane via barycentric coords
    double denom = 1.0 / (va + vb + vc);
    double v = vb * denom, w = vc * denom;
    double q[3] = {a[0] + ab[0] * v + ac[0] * w, a[1] + ab[1] * v + ac[1] * w,
                   a[2] + ab[2] * v + ac[2] * w};
    return dist2(q);
}

// Signed distance per cell: nearest boundary-triangle distance, signed by the
// inside/outside parity (negative inside). Brute force over triangles is acceptable for
// v1 (acceleration is an explicit non-goal).
std::vector<float> fill_distance(const Grid& g, const std::vector<Point3>& pts,
                                 const std::vector<Tri>& tris,
                                 const std::vector<std::uint8_t>& occ) {
    const std::size_t total = static_cast<std::size_t>(g.nx) * g.ny * g.nz;
    std::vector<float> dist(total, 0.0f);
    std::vector<std::array<double, 3>> tri_pts(tris.size() * 3);
    for (std::size_t t = 0; t < tris.size(); ++t)
        for (int v = 0; v < 3; ++v) {
            const Point3& p = pts[tris[t][v]];
            tri_pts[t * 3 + v] = {double(p.x), double(p.y), double(p.z)};
        }
    for (int k = 0; k < g.nz; ++k)
        for (int j = 0; j < g.ny; ++j)
            for (int i = 0; i < g.nx; ++i) {
                double center[3] = {g.corner[0] + (i + 0.5) * g.spacing,
                                    g.corner[1] + (j + 0.5) * g.spacing,
                                    g.corner[2] + (k + 0.5) * g.spacing};
                double best = std::numeric_limits<double>::max();
                for (std::size_t t = 0; t < tris.size(); ++t)
                    best = std::min(best, point_tri_dist2(center, tri_pts[t * 3].data(),
                                                          tri_pts[t * 3 + 1].data(),
                                                          tri_pts[t * 3 + 2].data()));
                std::size_t idx = (static_cast<std::size_t>(k) * g.ny + j) * g.nx + i;
                double d = std::sqrt(best);
                dist[idx] = static_cast<float>(occ[idx] ? -d : d);
            }
    return dist;
}

} // namespace

expected<VoxelGrid, MeshError> voxelize(const PLC& plc, const VoxelOptions& opts) {
    if (opts.resolution < 1)
        return unexpected(
            MeshError{MeshErrorCode::InvalidInput, "voxelize: resolution must be >= 1"});
    if (plc.points.empty())
        return unexpected(
            MeshError{MeshErrorCode::InvalidInput, "voxelize: PLC has no points"});
    std::vector<Tri> tris = boundary_triangles(plc);
    if (tris.empty())
        return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                    "voxelize: PLC has no boundary triangles"});

    robust::ensure_initialized();

    Grid g = make_grid(plc.points, opts);
    if (!(g.spacing > 0))
        return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                    "voxelize: degenerate (zero-extent) PLC"});

    auto buckets = build_xy_buckets(g, plc.points, tris);
    std::vector<std::uint8_t> occ = classify_occupancy(g, plc.points, tris, buckets);

    VoxelGrid out;
    out.spacing = g.spacing;
    out.nx = g.nx;
    out.ny = g.ny;
    out.nz = g.nz;
    out.origin = {static_cast<Real>(g.corner[0] + 0.5 * g.spacing),
                  static_cast<Real>(g.corner[1] + 0.5 * g.spacing),
                  static_cast<Real>(g.corner[2] + 0.5 * g.spacing)};

    if (opts.mode == VoxelMode::SignedDistance)
        out.distance = fill_distance(g, plc.points, tris, occ);
    else
        out.occupancy = std::move(occ);
    return out;
}

} // namespace cmg::voxelize
