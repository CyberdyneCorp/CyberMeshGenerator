// CyberMeshGenerator — typed wrapper over the ported Shewchuk predicates.
//
// The raw predicates live in cmg::predicates (a verbatim port of TetGen's
// predicates.cxx, compiled at -O0). This header provides a modern, typed surface:
// thread-safe one-time initialization, Point3 overloads, and the batched
// interface the accelerated hot path uses. Sign conventions are identical to
// TetGen's (orient3d > 0 iff pd is below the plane through pa,pb,pc, etc.).
#pragma once

#include <span>
#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/core/real.hpp"
#include "cmg/predicates/predicates.hpp" // raw ported Shewchuk predicates

namespace cmg::robust {

/// Initialize the exact predicates exactly once, thread-safely and idempotently.
/// Safe to call from any thread before evaluating a predicate; the mesher calls
/// it internally, but callers using the raw predicates should call it too.
void ensure_initialized();

/// Signed volume test: > 0, < 0, or 0 (coplanar), robustly. Matches TetGen.
double orient3d(const Point3& a, const Point3& b, const Point3& c,
                const Point3& d);

/// In-sphere test for the sphere through a,b,c,d evaluated at e. Matches TetGen.
double insphere(const Point3& a, const Point3& b, const Point3& c,
                const Point3& d, const Point3& e);

/// Batched orient3d over candidate tetrahedra given as index quadruples into
/// `pts`. Each returned sign equals the scalar orient3d for that candidate; the
/// device fast-filter path (when enabled) resolves uncertain candidates by exact
/// CPU escalation, so results are identical to per-item evaluation.
std::vector<double> orient3d_batch(std::span<const Point3> pts,
                                   std::span<const Tetrahedron> candidates);

} // namespace cmg::robust
