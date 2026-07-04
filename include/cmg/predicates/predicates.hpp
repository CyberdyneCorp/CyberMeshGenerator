#pragma once

// Shewchuk's public-domain adaptive-precision robust geometric predicates.
//
// Ported VERBATIM from TetGen's predicates.cxx (Jonathan Richard Shewchuk,
// "Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric
// Predicates", CMU-CS-96-140). The implementation in src/predicates/predicates.cpp
// is an unmodified transcription apart from namespacing and the REAL typedef.
//
// IMPORTANT: the .cpp MUST be compiled at -O0 (no fast-math / extended-precision
// reassociation) so that the exact arithmetic and error bounds remain valid.

// Pull in CMG_SINGLE so the REAL type is consistent across every translation
// unit (this header AND src/predicates/predicates.cpp), avoiding a float/double
// ABI mismatch between the ported predicates and their callers.
#include "cmg/backend/config.hpp"

namespace cmg::predicates {

// REAL is the floating-point type the predicates operate on. TetGen uses a
// `#define REAL double`; here it is a typedef selectable at compile time.
#ifdef CMG_SINGLE
using REAL = float;
#else
using REAL = double;
#endif

// Must be called once, before any predicate, to compute the machine epsilon
// and the static error bounds used by the adaptive routines.
void exactinit(int verbose, int noexact, int nofilter,
               REAL maxx, REAL maxy, REAL maxz);

// Adaptive-precision, robust geometric predicates. Each returns a value whose
// sign gives the orientation / in-circle / in-sphere result.
REAL orient2d(REAL* pa, REAL* pb, REAL* pc);
REAL orient3d(REAL* pa, REAL* pb, REAL* pc, REAL* pd);
REAL incircle(REAL* pa, REAL* pb, REAL* pc, REAL* pd);
REAL insphere(REAL* pa, REAL* pb, REAL* pc, REAL* pd, REAL* pe);
REAL orient4d(REAL* pa, REAL* pb, REAL* pc, REAL* pd, REAL* pe,
              REAL aheight, REAL bheight, REAL cheight, REAL dheight,
              REAL eheight);

} // namespace cmg::predicates
