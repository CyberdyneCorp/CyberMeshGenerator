// CyberMeshGenerator — mesh quality metrics and reporting.
#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "cmg/core/mesh.hpp"

namespace cmg::quality {

/// Number of buckets in the quality histograms.
inline constexpr int kHistogramBuckets = 18;

/// Aggregate shape/size quality statistics for a tetrahedral mesh.
struct QualityReport {
    std::size_t num_tets = 0;

    // Radius-edge ratio (circumradius / shortest edge); lower is better, ~0.612
    // for a regular tetrahedron.
    double min_radius_edge = 0, max_radius_edge = 0, mean_radius_edge = 0;

    // Dihedral angle in degrees; a regular tet is ~70.53°. Slivers have a very
    // small min or a very large max dihedral.
    double min_dihedral = 0, max_dihedral = 0, mean_dihedral = 0;

    // Tetrahedron volume.
    double min_volume = 0, max_volume = 0, total_volume = 0;

    // Dihedral-angle histogram over [0,180]° (kHistogramBuckets buckets).
    std::array<int, kHistogramBuckets> dihedral_histogram{};

    // Indices of the worst tetrahedra by radius-edge ratio (largest first),
    // capped at a small count for diagnostics.
    std::vector<int> worst_tets;
};

/// Compute the quality report for `mesh`. `worst_count` caps how many worst
/// tetrahedra are listed. Degenerate tets are handled without producing NaNs.
QualityReport report(const Mesh& mesh, int worst_count = 10);

} // namespace cmg::quality
