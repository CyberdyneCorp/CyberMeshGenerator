# constrained-tetrahedralization Specification (facet-recovery delta)

## ADDED Requirements

### Requirement: Conforming Delaunay facet recovery

CyberMeshGenerator SHALL, under `MeshOptions::preserve_facets`, recover the PLC's facets
by conforming Delaunay Steiner insertion: it SHALL add Steiner points (splitting
encroached subfaces / inserting subface circumcenters on the facet plane) until every
required facet subface appears as a face of the Delaunay tetrahedralization of the
augmented point set, or a Steiner budget is reached, and it SHALL then carve and emit a
mesh whose faces conform to the PLC facets. Facet recovery SHALL run after segment
recovery (so facet-boundary edges exist first) and SHALL be deterministic. On a convex or
star-shaped domain whose facets are already Delaunay faces, no Steiner points SHALL be
added. (oracle: TetGen constrained-tetrahedralization; manual §4.2.2 conforming Delaunay)

#### Scenario: A non-convex facet is recovered as mesh faces
- GIVEN a PLC with a facet that is not a face of the Delaunay tetrahedralization of its
  vertices
- WHEN it is tetrahedralized with `preserve_facets`
- THEN the facet's subfaces each appear as a union of faces of the output mesh, and the
  total mesh volume equals the domain volume within tolerance

#### Scenario: An internal facet that is a mesh face separates two regions
- GIVEN a PLC split by one internal facet that recovery leaves as a mesh face without the
  carve removing either side (e.g. a bipyramid whose wall is already a Delaunay face, so
  no wall Steiner points are needed), with a region seed on each side
- WHEN it is tetrahedralized with `preserve_facets`
- THEN the internal facet is present as mesh faces and the two cells receive distinct
  region markers (no tetrahedron straddles the facet)
- NOTE: **general** internal-facet separation — where recovering the wall requires Steiner
  points on it — is a non-goal of this increment (see the proposal): the carve counts
  internal facets in its point-in-domain ray cast and drops an interior cell, and
  conforming recovery does not terminate on a flat wall of coplanar points

#### Scenario: Convex input is unchanged
- GIVEN a convex PLC whose facets are already Delaunay faces
- WHEN it is tetrahedralized with `preserve_facets`
- THEN no Steiner points are added and the result equals the non-recovered mesh

#### Scenario: Budget is respected
- GIVEN a Steiner budget
- WHEN facet recovery cannot complete within it
- THEN recovery stops at the budget and reports incompleteness rather than looping
