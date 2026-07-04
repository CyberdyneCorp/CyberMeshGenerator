// CyberMeshGenerator — PLC self-intersection detection (TetGen -d).
#pragma once

#include <array>
#include <vector>

#include "cmg/core/plc.hpp"

namespace cmg::detect {

/// A detected intersection between two PLC facet triangles. `facet_a`/`facet_b`
/// index into the flattened list of facet triangles (each polygon fan-triangulated
/// in facet order), and the arrays give their vertex indices into `PLC::points`.
struct FacetIntersection {
    int facet_a = -1;
    int facet_b = -1;
    std::array<int, 3> tri_a{};
    std::array<int, 3> tri_b{};
};

/// Detect self-intersections in a PLC: pairs of non-adjacent facet triangles that
/// intersect (cross each other), using exact-predicate triangle-triangle tests.
/// Triangles that only share a vertex or an edge are NOT reported as
/// intersections. Returns all detected intersecting pairs (empty if the PLC is
/// clean). (oracle: TetGen self-intersection detection, -d)
std::vector<FacetIntersection> self_intersections(const PLC& plc);

} // namespace cmg::detect
