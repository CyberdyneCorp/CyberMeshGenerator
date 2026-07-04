// CyberMeshGenerator — segment (feature-edge) recovery (internal).
#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::recover {

struct SegmentResult {
    std::vector<Point3> points; ///< original points + edge Steiner points
    bool complete = false;      ///< true if every segment was recovered in budget
    std::size_t steiner_added = 0;
};

/// Insert Steiner points by bisection until every required segment (a pair of
/// indices into `points`) is covered by a chain of Delaunay edges. Returns the
/// augmented point set. Bounded by `budget` inserted points.
SegmentResult recover_segments(std::span<const Point3> points,
                               std::span<const std::array<int, 2>> segments,
                               std::size_t budget);

/// The unique undirected edges of every facet polygon of `plc`, as index pairs.
std::vector<std::array<int, 2>> facet_segments(const PLC& plc);

} // namespace cmg::recover
