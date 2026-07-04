# quality-mesh-generation Specification (metrics delta)

## ADDED Requirements

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
