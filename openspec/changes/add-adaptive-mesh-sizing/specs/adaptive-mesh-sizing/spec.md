# adaptive-mesh-sizing Specification

## ADDED Requirements

### Requirement: Analytic sizing function

CyberMeshGenerator SHALL accept an optional sizing function
`MeshOptions::sizing` returning the target edge length at a point, and SHALL refine
the mesh so tetrahedra are smaller where the target size is smaller. The target is
applied as a spatially-varying volume bound (`h³/(6√2)`) evaluated at each
tetrahedron's centroid; a returned size ≤ 0 SHALL leave that region unconstrained.
This is the idiomatic form of TetGen's programmatic `tetunsuitable` callback.
(oracle: TetGen adaptive-mesh-sizing, `-m`; tetgen.h:174,334)

#### Scenario: Graded refinement follows the size field
- GIVEN a box domain and a sizing function returning a small size in one half and a
  large size in the other
- WHEN it is tetrahedralized with that sizing function
- THEN the small-size region contains smaller (and more) tetrahedra than the
  large-size region

#### Scenario: Size zero leaves a region coarse
- GIVEN a sizing function returning 0 outside a sub-region
- WHEN the mesh is refined
- THEN tetrahedra outside the sub-region are not refined by the sizing function

### Requirement: Background-mesh sizing function

CyberMeshGenerator SHALL provide `cmg::sizing::from_background(background_mesh,
node_sizes, scale)` that builds a sizing function from a background tetrahedral mesh
carrying a per-node size, interpolating the target size at any query point from the
containing background tetrahedron (barycentric), scaled by `scale`. Points outside
the background mesh SHALL receive the nearest node's size. (oracle: TetGen
adaptive-mesh-sizing background mesh / `bgmin`; tetgen.cxx:3791)

#### Scenario: Interpolated sizing from a background mesh
- GIVEN a background mesh with a size assigned at each node
- WHEN a sizing function is built from it and used to refine a domain
- THEN the target size at each refinement point equals the barycentric interpolation
  of the background node sizes, and is exact at background nodes

#### Scenario: Metric scaling
- GIVEN a background sizing built with `scale = 0.5`
- WHEN it is used
- THEN every interpolated size is halved, producing a globally finer mesh

### Requirement: Sizing combines with the global volume bound

CyberMeshGenerator SHALL, when both `MeshOptions::sizing` and
`MeshOptions::max_volume` are set, refine each tetrahedron against the tighter of
the two targets; when neither is set, the mesh SHALL be the unrefined Phase 1–3
result.

#### Scenario: Tighter target wins
- GIVEN a sizing function and a global `max_volume`
- WHEN a tetrahedron's local sizing target is smaller than `max_volume` (or vice
  versa)
- THEN the smaller target governs that tetrahedron's refinement

#### Scenario: No sizing and no volume means no refinement
- GIVEN neither `sizing` nor `max_volume`
- WHEN the domain is tetrahedralized
- THEN no Steiner points are added
