// CyberMeshGenerator — Piecewise-Linear Complex (the input domain).
//
// Replaces TetGen's file-driven `.poly`/`.smesh` input and the facet/polygon
// arrays of `tetgenio`, as owning value types.
#pragma once

#include <vector>

#include "cmg/core/geometry.hpp"

namespace cmg {

/// A single polygon (a closed loop of vertex indices) within a facet.
struct Polygon {
    std::vector<Index> vertices;
};

/// A facet: one or more coplanar polygons (outer boundary + holes) plus optional
/// seed points marking sub-facet holes. Mirrors TetGen's `facet` -> `polygon`.
struct Facet {
    std::vector<Polygon> polygons;
    std::vector<Point3> holes; ///< seed points inside 2-D holes of this facet
    int marker = 0;            ///< boundary marker propagated to output faces
};

/// A region seed: a point inside a connected sub-domain carrying a material
/// attribute and an optional per-region maximum tetrahedron volume. Mirrors a
/// TetGen region list entry (x, y, z, attribute, max-volume).
struct Region {
    Point3 seed;
    Real attribute = 0;
    Real max_volume = -1; ///< negative means "unconstrained"
};

/// A Piecewise-Linear Complex: the polyhedral input domain to tetrahedralize.
class PLC {
public:
    std::vector<Point3> points;
    std::vector<Facet> facets;
    std::vector<Point3> holes;   ///< seed points inside volumetric holes
    std::vector<Region> regions; ///< material regions

    IndexBase index_base = IndexBase::Zero;

    bool empty() const noexcept { return points.empty() && facets.empty(); }
};

} // namespace cmg
