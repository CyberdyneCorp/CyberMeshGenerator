// CyberMeshGenerator — facet recovery by conforming Delaunay Steiner insertion.
//
// One dimension up from segment recovery (recover/segments.hpp): keep a work-list
// of subfaces (index triples). Each round, DT the augmented points, build the mesh
// FACE set, and split any subface that is not a Delaunay face until every required
// subface is a mesh face or the Steiner budget is exhausted. Deterministic.
#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include "cmg/core/geometry.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::recover {

struct FacetResult {
    std::vector<Point3> points;              ///< original points + facet Steiner points
    bool complete = false;                   ///< every subface recovered within budget
    std::size_t steiner_added = 0;
    std::vector<std::array<int, 3>> subfaces; ///< final leaf subfaces (DT faces on success)
};

/// Fan-triangulate every facet polygon (boundary AND internal) into subfaces, as
/// index triples into plc.points. Identical fan to cdt::boundary_triangles:
/// v[0], v[i-1], v[i] for i = 2..n. Order fixed by facet/polygon input order.
std::vector<std::array<int, 3>> plc_subfaces(const PLC& plc);

/// Insert Steiner points until every subface (an index triple into `points`) is a
/// face of the Delaunay tetrahedralization of the augmented point set, or `budget`
/// Steiner points have been added. A missing subface is split either at its in-plane
/// circumcenter (interior split) or, when that lies outside the triangle or would
/// encroach a protected boundary segment, at the midpoint of its longest edge
/// (edge split, shared across all subfaces on that edge). `segments` are the
/// already-recovered facet boundary segments to avoid encroaching (may be empty).
FacetResult recover_facets(std::span<const Point3> points,
                           std::span<const std::array<int, 3>> subfaces,
                           std::size_t budget,
                           std::span<const std::array<int, 2>> segments = {});

} // namespace cmg::recover
