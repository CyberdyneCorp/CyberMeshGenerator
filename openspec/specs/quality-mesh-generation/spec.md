# quality-mesh-generation Specification

## Purpose
TBD - created by archiving change add-quality-mesh-generation. Update Purpose after archive.
## Requirements
### Requirement: Maximum-volume refinement

When `MeshOptions::max_volume` is set, CyberMeshGenerator SHALL refine the mesh by
inserting Steiner points so that every output tetrahedron has volume ≤ the given
maximum (subject to the Steiner budget), while conserving the domain volume. The
refinement SHALL reuse the Delaunay / PLC-carving kernels each round and insert the
circumcenters of over-large tetrahedra that lie inside the domain. (oracle: TetGen
quality-mesh-generation, `-a#`; manual §4.2.4)

#### Scenario: Uniform volume bound on a cube
- GIVEN a cube PLC and `max_volume = 0.05`
- WHEN it is tetrahedralized
- THEN every output tetrahedron has volume ≤ 0.05, the tetrahedron count is larger
  than the unrefined mesh, and the total volume is still 1.0 within tolerance

### Requirement: Shape (radius-edge) refinement is deferred

CyberMeshGenerator SHALL accept `MeshOptions::quality` without error but SHALL NOT
yet perform shape/radius-edge refinement in this increment; setting only `quality`
(no `max_volume`) SHALL return the unrefined mesh. Radius-edge refinement by
circumcenter insertion diverges without sliver-exudation and boundary-encroachment
handling, so it is deferred to a later increment rather than shipped incorrectly.
(oracle: TetGen quality-mesh-generation `-q`; manual §4.2.3)

#### Scenario: Quality option accepted, shape refinement not yet applied
- GIVEN a domain meshed with only `quality.radius_edge` set (no `max_volume`)
- WHEN it is tetrahedralized
- THEN a valid unrefined mesh is returned without error and without divergence

### Requirement: Steiner-point budget

CyberMeshGenerator SHALL limit the total number of inserted Steiner points to
`MeshOptions::steiner_budget` when set; refinement SHALL stop once the budget is
exhausted and return the best mesh achieved, even if the quality/volume bounds are
not fully met, without error. (oracle: TetGen `-S#`; manual §4.2.3)

#### Scenario: Budget caps insertions
- GIVEN a demanding volume bound and `steiner_budget = 20`
- WHEN the mesh is refined
- THEN at most 20 Steiner points are added and a valid mesh is returned

### Requirement: Refinement is opt-in and domain-preserving

CyberMeshGenerator SHALL perform refinement only when `quality` or `max_volume` is
set; otherwise the mesh SHALL be exactly the unrefined Phase 1–3 result. Refinement
SHALL keep the mesh inside the domain and SHALL NOT change which domain is meshed.

#### Scenario: No options means no refinement
- GIVEN a domain meshed with neither `quality` nor `max_volume`
- WHEN it is tetrahedralized
- THEN the result is the unrefined mesh (no Steiner points added)

#### Scenario: Refinement stays inside the domain
- GIVEN a PLC refined with a volume bound
- WHEN refinement completes
- THEN the total volume equals the domain volume within tolerance (no tetrahedra
  outside the domain were introduced)

### Requirement: Mesh quality report

CyberMeshGenerator SHALL provide `cmg::quality::report(mesh, worst_count)` returning
a `QualityReport` with the min/max/mean radius-edge ratio, min/max/mean dihedral
angle in degrees, min/max/total tetrahedron volume, a dihedral-angle histogram over
[0,180]°, and the indices of the worst tetrahedra by radius-edge ratio (largest
first, capped at `worst_count`). Degenerate tetrahedra SHALL be handled without
producing NaN or infinite statistics. (oracle: TetGen quality statistics, `-V`)

#### Scenario: Report on a known mesh
- GIVEN the Delaunay mesh of a point set
- WHEN `report(mesh)` is computed
- THEN `num_tets` equals the tetrahedron count, the dihedral histogram bucket counts
  sum to `6 * num_tets` (six dihedrals per tet), `total_volume` equals the sum of
  tetrahedron volumes, and `min_dihedral <= mean_dihedral <= max_dihedral`

#### Scenario: Regular tetrahedron quality
- GIVEN a single near-regular tetrahedron
- WHEN its report is computed
- THEN its minimum dihedral angle is ~70.5° and its radius-edge ratio is ~0.61,
  within tolerance

### Requirement: Worst-element identification

CyberMeshGenerator SHALL list, in the quality report, the worst tetrahedra by
radius-edge ratio in descending order, so poor-quality elements can be located.

#### Scenario: Worst elements ranked
- GIVEN a mesh containing at least one poorly-shaped tetrahedron
- WHEN the report is computed
- THEN `worst_tets` is ordered by descending radius-edge ratio and its first entry
  is the tetrahedron with the largest ratio

