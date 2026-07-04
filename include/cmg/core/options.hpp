// CyberMeshGenerator — typed meshing options.
//
// Replaces TetGen's single-character switch string (e.g. "-pq1.414a0.1") and the
// 100+ fields of `tetgenbehavior` with a discoverable, defaulted, type-safe
// struct. A TetGen-compatible switch parser is offered as a compatibility layer.
#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/real.hpp"

namespace cmg {

/// Shape-quality bounds for refinement (TetGen -q). radius_edge is the maximum
/// radius-edge ratio; min_dihedral is the minimum dihedral angle in degrees.
struct Quality {
    Real radius_edge = 2.0;  ///< TetGen built-in default radius-edge ratio
    Real min_dihedral = 0.0; ///< minimum dihedral angle (degrees)
};

/// Predicate evaluation strategy (TetGen -X / -X1). Exact-adaptive is the correct
/// default; the others are diagnostic and may be wrong on degenerate input.
enum class PredicateMode {
    ExactAdaptive, ///< fast filter + exact escalation (default, robust)
    FloatingOnly,  ///< -X: floating-point estimate only (diagnostic)
    NoStaticFilter ///< -X1: keep exact escalation, drop the static filter
};

/// Typed meshing parameters. Every field maps to a documented TetGen switch but is
/// discoverable and defaulted rather than encoded in a character string.
struct MeshOptions {
    bool plc = false;               ///< -p: tetrahedralize a PLC
    bool preserve_surface = false;  ///< -Y: do not split input boundary faces
    bool reconstruct = false;       ///< -r: refine/reconstruct an existing mesh
    bool weighted = false;          ///< -w: weighted (regular) Delaunay
    bool convex = false;            ///< -c: retain the convex hull
    bool detect_intersections = false; ///< -d: report self-intersections
    bool label_regions = false;     ///< -AA: auto-label each connected region

    std::optional<Quality> quality;      ///< -q: enable quality refinement
    std::optional<Real> max_volume;      ///< -a#: global max tetrahedron volume
    std::optional<int> steiner_budget;   ///< -S#: cap on inserted Steiner points

    IndexBase index_base = IndexBase::Zero;           ///< -z: number from zero
    PredicateMode predicate_mode = PredicateMode::ExactAdaptive; ///< -X
    Real coplanar_tolerance = static_cast<Real>(1e-8); ///< -T#: epsilon

    bool emit_neighbors = false; ///< -n: populate Mesh::neighbors

    // BRIO-Hilbert spatial sort of the insertion order (-b analogue). Disabling
    // inserts in input order; the result stays a valid Delaunay mesh either way.
    bool spatial_sort = true;
    unsigned long long sort_seed = 1; ///< makes BRIO deterministic per run

    // Per-point weights for weighted (regular) Delaunay. Used only when
    // `weighted` is set; size must equal the point count. Each weight w lifts its
    // point to height x²+y²+z²−w.
    std::vector<Real> weights;

    // Adaptive mesh sizing (-m): target edge length at a point. Empty = unused; a
    // returned value <= 0 leaves that region unconstrained. Build one from a
    // background mesh with cmg::sizing::from_background, or supply any closure.
    std::function<double(const Point3&)> sizing;

    /// Parse a TetGen-style switch string (without the leading dash) into typed
    /// options. Returns a ParseError on an unknown or incompatible combination
    /// (e.g. weighted together with plc), never aborting the process.
    static expected<MeshOptions, ParseError> from_switches(std::string_view sw);
};

} // namespace cmg
