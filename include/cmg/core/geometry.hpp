// CyberMeshGenerator — basic geometric value types.
#pragma once

#include <array>
#include <cstddef>

#include "cmg/core/real.hpp"

namespace cmg {

/// A point / vertex in 3-D space. Replaces TetGen's flat `REAL* pointlist`.
struct Point3 {
    Real x = 0;
    Real y = 0;
    Real z = 0;

    constexpr Real operator[](std::size_t i) const noexcept {
        return (&x)[i];
    }
    constexpr Real& operator[](std::size_t i) noexcept { return (&x)[i]; }

    friend constexpr bool operator==(const Point3&, const Point3&) = default;
};

/// Index type for referring to vertices / tetrahedra / faces within a Mesh/PLC.
using Index = int;

/// A tetrahedron as four vertex indices into the owning Mesh's point array.
using Tetrahedron = std::array<Index, 4>;

/// A triangular face as three vertex indices.
using Triangle = std::array<Index, 3>;

/// Whether object indices start at 0 or 1. TetGen auto-detects this per input;
/// the port makes it an explicit, typed choice (mirrors the `-z`/first-number
/// behavior).
enum class IndexBase { Zero, One };

} // namespace cmg
