// CyberMeshGenerator — the REAL number type.
#pragma once

#include "cmg/backend/config.hpp"

namespace cmg {

// Default is double precision, matching TetGen (`#define REAL double`).
// A CMG_SINGLE build selects float to save memory on constrained mobile targets,
// mirroring TetGen's -DSINGLE. This alias MUST agree with the REAL used by the
// ported predicates translation unit (cmg::predicates::REAL).
#ifdef CMG_SINGLE
using Real = float;
#else
using Real = double;
#endif

} // namespace cmg
