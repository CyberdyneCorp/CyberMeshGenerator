// CyberMeshGenerator — tests for cmg::reconstruct::reconstruct.
#include "cmg/reconstruct/reconstruct.hpp"

#include <cmath>
#include <vector>

#include "cmg/cmg.hpp"
#include "harness.hpp"

namespace {

std::vector<cmg::Point3> cube_corners() {
    return {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
            {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
}

// Signed volume magnitude of a tetrahedron: |det(v1-v0, v2-v0, v3-v0)| / 6.
double tet_volume(const cmg::Mesh& m, const cmg::Tetrahedron& t) {
    const cmg::Point3& a = m.points[t[0]];
    const cmg::Point3& b = m.points[t[1]];
    const cmg::Point3& c = m.points[t[2]];
    const cmg::Point3& d = m.points[t[3]];
    const double bx = b.x - a.x, by = b.y - a.y, bz = b.z - a.z;
    const double cx = c.x - a.x, cy = c.y - a.y, cz = c.z - a.z;
    const double dx = d.x - a.x, dy = d.y - a.y, dz = d.z - a.z;
    const double det = bx * (cy * dz - cz * dy) - by * (cx * dz - cz * dx) +
                       bz * (cx * dy - cy * dx);
    return std::fabs(det) / 6.0;
}

} // namespace

CMG_TEST("reconstruct refines to max_volume and adds tets") {
    const auto corners = cube_corners();
    auto base = cmg::delaunay(std::span<const cmg::Point3>(corners), {});
    CMG_CHECK(base.has_value());
    const cmg::Mesh& m = *base;

    cmg::MeshOptions opts;
    opts.max_volume = 0.05;
    auto r = cmg::reconstruct::reconstruct(m, opts);
    CMG_CHECK(r.has_value());

    CMG_CHECK(r->tet_count() > m.tet_count());
    for (const auto& t : r->tetrahedra) {
        CMG_CHECK(tet_volume(*r, t) <= 0.05 + 1e-6);
    }
}

CMG_TEST("reconstruct retains all input vertices") {
    const auto corners = cube_corners();
    auto base = cmg::delaunay(std::span<const cmg::Point3>(corners), {});
    CMG_CHECK(base.has_value());

    auto r = cmg::reconstruct::reconstruct(*base, {});
    CMG_CHECK(r.has_value());
    CMG_CHECK(r->point_count() >= 8);
}
