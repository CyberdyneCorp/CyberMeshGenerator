# constrained-tetrahedralization Specification (segment recovery delta)

## ADDED Requirements

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
