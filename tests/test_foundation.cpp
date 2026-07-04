// CyberMeshGenerator — foundation tests.
//
// Exercises the typed API end-to-end on the CPU-only build: robust predicates,
// the switch-string compatibility parser, the error/result model, and the
// irreducible single-tetrahedron base case.
#include "cmg/cmg.hpp"
#include "harness.hpp"

#include <array>
#include <vector>

using namespace cmg;

CMG_TEST("orient3d gives a non-zero, sign-consistent result") {
    Point3 a{0, 0, 0}, b{1, 0, 0}, c{0, 1, 0}, d{0, 0, 1};
    const double s = robust::orient3d(a, b, c, d);
    CMG_CHECK(s != 0.0);
    // Swapping two vertices flips the orientation sign.
    const double s2 = robust::orient3d(a, b, d, c);
    CMG_CHECK((s > 0) != (s2 > 0));
}

CMG_TEST("insphere detects a point inside/outside the circumsphere") {
    Point3 a{0, 0, 0}, b{1, 0, 0}, c{0, 1, 0}, d{0, 0, 1};
    Point3 inside{0.25, 0.25, 0.25};
    Point3 far{100, 100, 100};
    // Opposite signs: one point is inside the sphere, the other outside.
    const double si = robust::insphere(a, b, c, d, inside);
    const double so = robust::insphere(a, b, c, d, far);
    CMG_CHECK((si > 0) != (so > 0));
}

CMG_TEST("switch parser maps -pq1.414a0.1 to typed options") {
    auto r = MeshOptions::from_switches("pq1.414a0.1");
    CMG_CHECK(bool(r));
    CMG_CHECK(r->plc);
    CMG_CHECK(r->quality.has_value());
    CMG_CHECK(r->quality->radius_edge > 1.413 && r->quality->radius_edge < 1.415);
    CMG_CHECK(r->max_volume.has_value());
    CMG_CHECK(*r->max_volume > 0.09 && *r->max_volume < 0.11);
    CMG_CHECK(r->quality.has_value()); // -a implies -q
}

CMG_TEST("switch parser rejects weighted + PLC as a ParseError") {
    auto r = MeshOptions::from_switches("pw");
    CMG_CHECK(!r);
    CMG_CHECK(!r.error().message.empty());
}

CMG_TEST("delaunay of four non-coplanar points yields one tetrahedron") {
    std::vector<Point3> pts{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    auto r = delaunay(pts, {});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_count() == 1);
    CMG_CHECK(r->face_count() == 4);
    CMG_CHECK(r->point_count() == 4);
    // The stored tetrahedron is in TetGen's canonical (negative) orientation.
    const auto& t = r->tetrahedra[0];
    CMG_CHECK(robust::orient3d(r->points[t[0]], r->points[t[1]],
                               r->points[t[2]], r->points[t[3]]) < 0);
}

CMG_TEST("delaunay of coplanar points is an InvalidInput error") {
    std::vector<Point3> pts{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    auto r = delaunay(pts, {});
    CMG_CHECK(!r);
    CMG_CHECK(r.error().code == MeshErrorCode::InvalidInput);
}

CMG_TEST("fewer than four points is an InvalidInput error, not a crash") {
    std::vector<Point3> pts{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    auto r = delaunay(pts, {});
    CMG_CHECK(!r);
    CMG_CHECK(r.error().code == MeshErrorCode::InvalidInput);
}

CMG_TEST("five-point set tetrahedralizes via the Phase 1 kernel") {
    // Phase 0 returned NotImplemented here; Phase 1's incremental kernel now
    // produces a real mesh for n > 4.
    std::vector<Point3> pts{{0, 0, 0}, {1, 0, 0}, {0, 1, 0},
                            {0, 0, 1}, {1, 1, 1}};
    auto r = delaunay(pts, {});
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tet_count() >= 2);
}

CMG_TEST("empty PLC is rejected") {
    PLC plc;
    auto r = tetrahedralize(plc, MeshOptions{.plc = true});
    CMG_CHECK(!r);
    CMG_CHECK(r.error().code == MeshErrorCode::InvalidInput);
}

CMG_TEST("orient3d_batch equals per-item evaluation") {
    std::vector<Point3> pts{{0, 0, 0}, {1, 0, 0}, {0, 1, 0},
                            {0, 0, 1}, {0, 0, -1}};
    std::vector<Tetrahedron> cands{{0, 1, 2, 3}, {0, 1, 2, 4}};
    auto batch = robust::orient3d_batch(pts, cands);
    CMG_CHECK(batch.size() == 2);
    for (std::size_t i = 0; i < cands.size(); ++i) {
        const auto& c = cands[i];
        const double one =
            robust::orient3d(pts[c[0]], pts[c[1]], pts[c[2]], pts[c[3]]);
        CMG_CHECK(batch[i] == one);
    }
}

CMG_TEST("backend on a CPU-only build reports cpu") {
    // Below threshold, or with no device, offloading never happens.
    CMG_CHECK(!backend::should_offload(backend::Op::OrientFilter, 10));
    CMG_CHECK(backend::last_backend() == backend::Backend::Cpu);
    CMG_CHECK(backend::name(backend::Backend::Cpu) == "cpu");
}

int main() {
    std::printf("CyberMeshGenerator foundation tests\n");
    return cmgtest::run_all();
}
