// CyberMeshGenerator — background-mesh sizing function implementation.
#include "cmg/sizing/background.hpp"

#include <limits>
#include <memory>
#include <vector>

#include "cmg/predicates/robust.hpp"

namespace cmg::sizing {

namespace {

// Shared, immutable background data captured by the returned closure.
struct BgData {
    std::vector<Point3> points;
    std::vector<Tetrahedron> tets;
    std::vector<double> sizes;
    double scale;
};

double dist2(const Point3& a, const Point3& b) {
    double dx = double(a.x) - b.x, dy = double(a.y) - b.y, dz = double(a.z) - b.z;
    return dx * dx + dy * dy + dz * dz;
}

// Barycentric interpolation of node sizes at p inside tet t; returns false if p
// is not inside t. Tets are stored in the library's orient3d(v0..v3) < 0 form.
bool interp_in_tet(const BgData& bg, const Tetrahedron& t, const Point3& p,
                   double& out) {
    const Point3 &v0 = bg.points[t[0]], &v1 = bg.points[t[1]],
                 &v2 = bg.points[t[2]], &v3 = bg.points[t[3]];
    double D = robust::orient3d(v0, v1, v2, v3);
    if (D == 0) return false;
    double w0 = robust::orient3d(p, v1, v2, v3) / D;
    double w1 = robust::orient3d(v0, p, v2, v3) / D;
    double w2 = robust::orient3d(v0, v1, p, v3) / D;
    double w3 = robust::orient3d(v0, v1, v2, p) / D;
    const double eps = -1e-9;
    if (w0 < eps || w1 < eps || w2 < eps || w3 < eps) return false;
    out = w0 * bg.sizes[t[0]] + w1 * bg.sizes[t[1]] + w2 * bg.sizes[t[2]] +
          w3 * bg.sizes[t[3]];
    return true;
}

double nearest_size(const BgData& bg, const Point3& p) {
    double best = std::numeric_limits<double>::max();
    std::size_t bi = 0;
    for (std::size_t i = 0; i < bg.points.size(); ++i) {
        double d = dist2(bg.points[i], p);
        if (d < best) { best = d; bi = i; }
    }
    return bg.sizes.empty() ? 0.0 : bg.sizes[bi];
}

} // namespace

std::function<double(const Point3&)> from_background(
    const Mesh& background, std::span<const double> node_sizes, double scale) {
    auto bg = std::make_shared<BgData>();
    bg->points = background.points;
    bg->tets = background.tetrahedra;
    bg->sizes.assign(node_sizes.begin(), node_sizes.end());
    bg->scale = scale;
    // Size vector must match the node count; pad/truncate defensively.
    bg->sizes.resize(bg->points.size(), 0.0);

    return [bg](const Point3& p) -> double {
        double s;
        for (const Tetrahedron& t : bg->tets)
            if (interp_in_tet(*bg, t, p, s)) return s * bg->scale;
        return nearest_size(*bg, p) * bg->scale; // outside the background mesh
    };
}

} // namespace cmg::sizing
