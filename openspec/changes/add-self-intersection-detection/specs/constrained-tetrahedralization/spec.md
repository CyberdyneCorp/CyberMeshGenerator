# constrained-tetrahedralization Specification (self-intersection delta)

## ADDED Requirements

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
