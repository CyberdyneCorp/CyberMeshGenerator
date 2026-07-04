# mesh-coarsening Specification

## ADDED Requirements

### Requirement: Coarsen by interior-vertex removal

CyberMeshGenerator SHALL provide `cmg::coarsen::coarsen(mesh, opts)` that removes a
deterministic fraction of the mesh's interior vertices (those on no boundary face)
and re-tetrahedralizes the remaining vertices, returning a valid tetrahedral mesh.
Every boundary vertex SHALL be retained so the domain is preserved, and the result
SHALL have no more tetrahedra than the input for `keep_fraction < 1`. (oracle: TetGen
mesh-coarsening, `-R`; manual §4.2.5)

#### Scenario: Fewer elements, boundary preserved
- GIVEN a refined mesh with interior vertices and `keep_fraction = 0.3`
- WHEN it is coarsened
- THEN the result is a valid mesh with fewer tetrahedra than the input and every
  boundary vertex retained

#### Scenario: Deterministic for a fixed seed
- GIVEN a fixed `seed`
- WHEN the same mesh is coarsened twice
- THEN both results are identical
