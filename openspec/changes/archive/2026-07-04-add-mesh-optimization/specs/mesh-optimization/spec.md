# mesh-optimization Specification

## ADDED Requirements

### Requirement: Laplacian smoothing of interior vertices

CyberMeshGenerator SHALL provide `cmg::optimize::laplacian_smooth(mesh, opts)` that
relocates each interior vertex (one belonging to no boundary face) toward the
centroid of its edge-neighbors over `opts.iterations` sweeps, and SHALL keep every
boundary vertex fixed. A proposed relocation that would invert any tetrahedron
incident to the vertex (flip its orientation sign) SHALL be damped or skipped so the
mesh never becomes inverted. (oracle: TetGen mesh-optimization, `-O`; manual §4.2.6)

#### Scenario: Boundary preserved, no inversions, volume conserved
- GIVEN a valid tetrahedral mesh of a domain
- WHEN it is Laplacian-smoothed
- THEN every boundary vertex is unchanged, every output tetrahedron has the same
  orientation sign as before (none inverted), and the total volume is unchanged
  within tolerance

#### Scenario: Interior vertices move toward their neighbor centroid
- GIVEN an interior vertex not at the centroid of its neighbors
- WHEN one smoothing sweep runs with full relaxation and the move does not invert a
  tetrahedron
- THEN the vertex is closer to its neighbors' centroid than before

### Requirement: Dihedral-angle quality measure

CyberMeshGenerator SHALL provide `cmg::optimize::min_dihedral_angle(mesh)` returning
the minimum dihedral angle in degrees over all tetrahedra, and smoothing SHALL NOT
decrease this measure below its input value for a mesh it can improve. (oracle: TetGen
mesh quality reporting)

#### Scenario: Smoothing does not worsen the worst angle
- GIVEN a mesh with a measurable minimum dihedral angle
- WHEN it is Laplacian-smoothed
- THEN the minimum dihedral angle of the result is greater than or equal to the
  input's (it does not get worse)
