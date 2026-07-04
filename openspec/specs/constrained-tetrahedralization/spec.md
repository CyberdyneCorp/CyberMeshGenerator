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

### Requirement: Segment recovery by bisection

CyberMeshGenerator SHALL provide `cmg::recover::recover_segments(points, segments,
budget)` that inserts Steiner points by bisection until every required segment is
covered by a chain of Delaunay edges, returning the augmented point set and whether
recovery completed within the budget. A segment that is already a Delaunay edge SHALL
receive no Steiner points. (oracle: TetGen constrained-tetrahedralization, segment
recovery; manual §4.2.2)

#### Scenario: A non-Delaunay segment is recovered as an edge chain
- GIVEN a point set in which a required segment is not a Delaunay edge
- WHEN `recover_segments` is run on it
- THEN the returned augmented point set's Delaunay tetrahedralization contains a chain
  of edges covering the segment, and at least one Steiner point was added

#### Scenario: Already-present segments are untouched
- GIVEN required segments that are already Delaunay edges (e.g. a cube's edges)
- WHEN `recover_segments` is run
- THEN no Steiner points are added and recovery completes

#### Scenario: Budget bounds a hard recovery
- GIVEN a required segment and a Steiner budget of N
- WHEN recovery cannot satisfy it within N points
- THEN at most N Steiner points are added and the result reports incomplete, without
  looping

### Requirement: Feature-edge preservation for a PLC

Under `MeshOptions::preserve_edges`, CyberMeshGenerator SHALL recover every facet edge
of the PLC before carving, so that each facet edge appears as a chain of mesh edges in
the output tetrahedralization. When `preserve_edges` is off, the mesh SHALL be
unchanged from the non-recovering pipeline. (oracle: TetGen boundary preservation, `-Y`)

#### Scenario: Facet edges present in the meshed PLC
- GIVEN a PLC and `preserve_edges` set
- WHEN it is tetrahedralized
- THEN every edge of every facet polygon is covered by a chain of mesh edges

#### Scenario: Preserve-edges is opt-in
- GIVEN a PLC meshed without `preserve_edges`
- WHEN it is tetrahedralized
- THEN no segment-recovery Steiner points are added

### Requirement: PLC self-intersection detection

CyberMeshGenerator SHALL provide `cmg::detect::self_intersections(plc)` returning the
pairs of PLC facet triangles that intersect, using exact-predicate triangle-triangle
intersection tests over all non-adjacent facet triangles (each facet polygon
fan-triangulated). Triangle pairs that only share a vertex or an edge SHALL NOT be
reported. The result SHALL be empty for a valid, non-self-intersecting PLC. (oracle:
TetGen self-intersection detection, -d; manual §4.2.8)

#### Scenario: Crossing facets detected
- GIVEN a PLC containing two facet triangles that pass through each other
- WHEN `self_intersections` is called
- THEN the intersecting pair is reported

#### Scenario: A valid closed surface is clean
- GIVEN a valid closed PLC (e.g. a cube or tetrahedron surface) whose facets meet
  only along shared edges
- WHEN `self_intersections` is called
- THEN the result is empty (edge/vertex-sharing is not an intersection)

