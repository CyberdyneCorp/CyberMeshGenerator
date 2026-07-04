// CyberMeshGenerator — tetrahedron circumcenter (shared geometry helper).
#pragma once

#include <algorithm>
#include <cmath>

#include "cmg/core/geometry.hpp"

namespace cmg::geom {

/// Circumcenter of tetrahedron a,b,c,d — the point equidistant from all four. On
/// success sets `center` and `radius` (the circumradius) and returns true; returns
/// false for a near-degenerate (near-zero-volume) tetrahedron or a non-finite
/// result. Shared by quality refinement and the Voronoi construction.
inline bool circumcenter(const Point3& a, const Point3& b, const Point3& c,
                         const Point3& d, Point3& center, double& radius) {
    const double B[3] = {double(b.x) - a.x, double(b.y) - a.y, double(b.z) - a.z};
    const double C[3] = {double(c.x) - a.x, double(c.y) - a.y, double(c.z) - a.z};
    const double D[3] = {double(d.x) - a.x, double(d.y) - a.y, double(d.z) - a.z};
    auto cross = [](const double u[3], const double v[3], double out[3]) {
        out[0] = u[1] * v[2] - u[2] * v[1];
        out[1] = u[2] * v[0] - u[0] * v[2];
        out[2] = u[0] * v[1] - u[1] * v[0];
    };
    auto dot = [](const double u[3], const double v[3]) {
        return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
    };
    double CD[3], DB[3], BC[3];
    cross(C, D, CD);
    cross(D, B, DB);
    cross(B, C, BC);
    const double denom = 2.0 * dot(B, CD);
    const double nB = dot(B, B), nC = dot(C, C), nD = dot(D, D);
    const double scale = std::sqrt(std::max({nB, nC, nD, 1e-300}));
    if (std::fabs(denom) < 1e-12 * scale * scale * scale) return false;
    double num[3];
    for (int i = 0; i < 3; i++)
        num[i] = (nB * CD[i] + nC * DB[i] + nD * BC[i]) / denom;
    center = {static_cast<Real>(a.x + num[0]), static_cast<Real>(a.y + num[1]),
              static_cast<Real>(a.z + num[2])};
    radius = std::sqrt(num[0] * num[0] + num[1] * num[1] + num[2] * num[2]);
    return std::isfinite(center.x) && std::isfinite(center.y) &&
           std::isfinite(center.z) && std::isfinite(radius);
}

} // namespace cmg::geom
