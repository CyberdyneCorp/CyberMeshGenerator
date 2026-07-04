// CyberMeshGenerator — typed predicate wrapper implementation.
#include "cmg/predicates/robust.hpp"

#include <mutex>

namespace cmg::robust {

namespace {

std::once_flag g_init_flag;

// Copy a Point3's coordinates into a mutable buffer: the ported predicates take
// REAL* (non-const) even though they never modify their inputs.
inline void to_buf(const Point3& p, predicates::REAL out[3]) {
    out[0] = static_cast<predicates::REAL>(p.x);
    out[1] = static_cast<predicates::REAL>(p.y);
    out[2] = static_cast<predicates::REAL>(p.z);
}

} // namespace

void ensure_initialized() {
    // verbose=0, noexact=0 (keep exact escalation), nofilter=0 (keep the static
    // filter). max* are only used for an optional CGAL path we don't enable.
    std::call_once(g_init_flag, [] {
        predicates::exactinit(0, 0, 0, 0, 0, 0);
    });
}

double orient3d(const Point3& a, const Point3& b, const Point3& c,
                const Point3& d) {
    ensure_initialized();
    predicates::REAL pa[3], pb[3], pc[3], pd[3];
    to_buf(a, pa); to_buf(b, pb); to_buf(c, pc); to_buf(d, pd);
    return static_cast<double>(predicates::orient3d(pa, pb, pc, pd));
}

double insphere(const Point3& a, const Point3& b, const Point3& c,
                const Point3& d, const Point3& e) {
    ensure_initialized();
    predicates::REAL pa[3], pb[3], pc[3], pd[3], pe[3];
    to_buf(a, pa); to_buf(b, pb); to_buf(c, pc); to_buf(d, pd); to_buf(e, pe);
    return static_cast<double>(predicates::insphere(pa, pb, pc, pd, pe));
}

std::vector<double> orient3d_batch(std::span<const Point3> pts,
                                   std::span<const Tetrahedron> candidates) {
    ensure_initialized();
    std::vector<double> out;
    out.reserve(candidates.size());
    // Foundation: the exact per-candidate CPU path. The device fast-filter is
    // registered by the backend layer when a GPU backend is compiled in; it only
    // pre-screens obvious signs and defers uncertain candidates to this exact
    // path, so the result is identical either way.
    for (const Tetrahedron& t : candidates) {
        out.push_back(
            orient3d(pts[t[0]], pts[t[1]], pts[t[2]], pts[t[3]]));
    }
    return out;
}

} // namespace cmg::robust
