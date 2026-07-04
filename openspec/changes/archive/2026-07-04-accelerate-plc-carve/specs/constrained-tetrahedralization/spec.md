# constrained-tetrahedralization Specification (carve-acceleration delta)

## MODIFIED Requirements

### Requirement: Boundary-conforming tetrahedralization of a PLC domain

CyberMeshGenerator SHALL tetrahedralize the interior of a domain bounded by a
Piecewise-Linear Complex, returning a `Mesh` of the tetrahedra inside the domain.
The mesh SHALL be the Delaunay tetrahedralization of the PLC vertices restricted to
the domain interior, with tetrahedra outside the domain (and inside facet-bounded
voids) removed. For a convex or star-shaped domain the result SHALL conserve the
domain volume (the sum of tetrahedron volumes equals the domain volume within
tolerance) and its boundary SHALL lie on the PLC surface. `tetrahedralize(PLC)` on
a faceted PLC SHALL route to this pipeline.

The interior classification SHALL use a spatial acceleration structure over the
boundary triangles so that its cost scales sub-linearly in boundary-triangle count
rather than testing every triangle per candidate tetrahedron, and the classification
result SHALL be identical to testing every triangle (a full-resolution boundary yields
the same mesh as before, only faster). (oracle: TetGen constrained-tetrahedralization;
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

#### Scenario: Full-resolution boundary is tractable and unchanged
- GIVEN a closed surface with many boundary triangles (thousands or more)
- WHEN it is tetrahedralized without prior decimation
- THEN the carve completes using the spatial index, and the kept mesh is identical to
  the mesh obtained by testing every boundary triangle
