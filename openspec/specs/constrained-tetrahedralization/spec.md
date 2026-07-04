# constrained-tetrahedralization Specification

## Purpose
TBD - created by archiving change add-constrained-tetrahedralization. Update Purpose after archive.
## Requirements
### Requirement: Boundary-conforming tetrahedralization of a PLC domain

CyberMeshGenerator SHALL tetrahedralize the interior of a domain bounded by a
Piecewise-Linear Complex, returning a `Mesh` of the tetrahedra inside the domain.
The mesh SHALL be the Delaunay tetrahedralization of the PLC vertices restricted to
the domain interior, with tetrahedra outside the domain (and inside facet-bounded
voids) removed. For a convex or star-shaped domain the result SHALL conserve the
domain volume (the sum of tetrahedron volumes equals the domain volume within
tolerance) and its boundary SHALL lie on the PLC surface. `tetrahedralize(PLC)` on
a faceted PLC SHALL route to this pipeline. (oracle: TetGen constrained-tetrahedralization;
manual §4.2.2, `-p` default)

#### Scenario: Convex solid is meshed with conserved volume
- GIVEN a convex PLC (e.g. a cube given by its triangulated boundary)
- WHEN `tetrahedralize(plc, {.plc = true})` is called
- THEN a valid tetrahedral mesh of the interior is returned whose total volume
  equals the domain volume within tolerance

#### Scenario: Single tetrahedron domain
- GIVEN a PLC that is a tetrahedron (four triangular facets)
- WHEN it is tetrahedralized
- THEN the result is a single tetrahedron

#### Scenario: Exterior tetrahedra are carved away
- GIVEN a PLC whose convex hull is strictly larger than the domain (a facet-bounded
  concavity or void)
- WHEN it is tetrahedralized
- THEN tetrahedra outside the domain are removed and the kept volume is less than
  the convex-hull volume

### Requirement: Domain boundary face output

The pipeline SHALL emit in `Mesh::faces` the faces of the kept tetrahedra that lie
on the domain boundary (each bordering exactly one kept tetrahedron), with boundary
marker 1, and SHALL NOT emit interior faces. (oracle: TetGen `.face` default output)

#### Scenario: Boundary faces bound the kept mesh
- WHEN a PLC domain is tetrahedralized
- THEN every emitted face borders exactly one kept tetrahedron and lies on the
  domain boundary, marked 1

### Requirement: Recoverable failure on a degenerate PLC

The pipeline SHALL return a `MeshError` (never throw or terminate) when the PLC
domain cannot be tetrahedralized — fewer than four non-coplanar vertices, or an
all-coplanar vertex set. (oracle: mesh-core-foundation error model)

#### Scenario: Degenerate PLC reported
- GIVEN a PLC whose vertices are all coplanar
- WHEN it is tetrahedralized
- THEN a `MeshError` with code `InvalidInput` is returned

