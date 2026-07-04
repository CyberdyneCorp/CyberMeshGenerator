// CyberMeshGenerator — backend dispatch / GPU-filter tests.
#include "cmg/cmg.hpp"
#include "harness.hpp"

#include <array>
#include <vector>

using namespace cmg;

namespace {

std::vector<Point3> cloud(int n, unsigned s) {
    std::vector<Point3> p;
    auto nx = [&] { s = s * 1664525u + 1013904223u; return static_cast<Real>((s >> 8) / double(1u << 24)); };
    for (int i = 0; i < n; ++i) p.push_back({nx(), nx(), nx()});
    return p;
}

int exact_sign(const std::vector<Point3>& pts, const Tetrahedron& t) {
    double v = robust::orient3d(pts[t[0]], pts[t[1]], pts[t[2]], pts[t[3]]);
    return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

} // namespace

CMG_TEST("orient3d_signs matches the exact per-candidate sign (CPU path)") {
    auto pts = cloud(30, 5);
    std::vector<Tetrahedron> cands;
    for (int i = 0; i + 3 < 30; i += 1) cands.push_back({i, i + 1, i + 2, i + 3});
    auto signs = robust::orient3d_signs(pts, cands);
    CMG_CHECK(signs.size() == cands.size());
    for (std::size_t i = 0; i < cands.size(); ++i)
        CMG_CHECK(signs[i] == exact_sign(pts, cands[i]));
}

#if CMG_WITH_CUDA
CMG_TEST("CUDA orient3d_signs: large batch matches CPU exact, backend=cuda") {
    // A batch above the offload threshold so the GPU fast-filter is used.
    auto pts = cloud(2000, 9);
    std::vector<Tetrahedron> cands;
    unsigned s = 123;
    auto rnd = [&](int n) { s = s * 1664525u + 1013904223u; return int((s >> 8) % n); };
    for (int i = 0; i < 100000; ++i) {
        int a = rnd(2000), b = rnd(2000), c = rnd(2000), d = rnd(2000);
        cands.push_back({a, b, c, d});
    }
    auto signs = robust::orient3d_signs(pts, cands);
    CMG_CHECK(backend::last_backend() == backend::Backend::Cuda);
    bool all_match = true;
    for (std::size_t i = 0; i < cands.size(); ++i)
        if (signs[i] != exact_sign(pts, cands[i])) { all_match = false; break; }
    CMG_CHECK(all_match); // GPU + exact escalation == pure CPU
}
#endif
