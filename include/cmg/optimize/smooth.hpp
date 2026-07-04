// CyberMeshGenerator — mesh optimization (Laplacian smoothing + quality metrics).
#pragma once

#include "cmg/core/mesh.hpp"

namespace cmg::optimize {

struct SmoothOptions {
    int iterations = 5;       ///< number of smoothing sweeps
    double relaxation = 1.0;  ///< 1.0 = move fully to the neighbor centroid
};

/// Laplacian-smooth the interior vertices of `mesh` (those not on any boundary
/// face) toward the centroid of their edge-neighbors, never inverting an incident
/// tetrahedron (a proposed move that would flip any incident tet's orientation is
/// damped or skipped). Boundary vertices are fixed, so the domain — and the total
/// volume — is preserved. Returns the improved mesh.
Mesh laplacian_smooth(const Mesh& mesh, const SmoothOptions& opts = {});

/// The minimum dihedral angle (degrees) over all tetrahedra — a standard shape
/// quality measure; larger is better (a regular tetrahedron is ~70.5°).
double min_dihedral_angle(const Mesh& mesh);

} // namespace cmg::optimize
