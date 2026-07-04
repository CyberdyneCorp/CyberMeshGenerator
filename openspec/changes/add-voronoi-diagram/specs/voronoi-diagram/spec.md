# voronoi-diagram Specification

## ADDED Requirements

### Requirement: Voronoi vertices are Delaunay circumcenters

CyberMeshGenerator SHALL provide `cmg::voronoi::build(const Mesh&)` returning a
Voronoi diagram whose vertices are the circumcenters of the Delaunay tetrahedra,
one per tetrahedron. Each Voronoi vertex SHALL be equidistant from the four vertices
of its tetrahedron within tolerance. (oracle: TetGen voronoi-diagram, `-v`;
manual §5.2.10)

#### Scenario: One equidistant circumcenter per tetrahedron
- GIVEN the Delaunay tetrahedralization of a point set
- WHEN the Voronoi diagram is built
- THEN it has exactly one vertex per tetrahedron, and each is equidistant from that
  tetrahedron's four vertices within tolerance

### Requirement: Voronoi edges are dual to Delaunay faces

CyberMeshGenerator SHALL emit one Voronoi edge per Delaunay face: a finite edge
between the two adjacent tetrahedra's circumcenters for an interior face, and a ray
(`v1 = -1`) with an outward unit direction for a convex-hull face. (oracle: TetGen
voronoi-diagram; manual §5.2.10)

#### Scenario: Interior faces give finite edges, hull faces give rays
- GIVEN a Delaunay tetrahedralization
- WHEN the Voronoi diagram is built
- THEN each interior face yields a finite edge joining two circumcenters, and each
  convex-hull face yields a ray whose direction points outward from the mesh

### Requirement: Voronoi cells are dual to input vertices

CyberMeshGenerator SHALL provide, per input vertex, the set of incident Voronoi
vertices (the circumcenters of the tetrahedra incident to that input vertex). Every
input vertex used by the mesh SHALL have a non-empty cell. (oracle: TetGen
voronoi-diagram `.v.cell`; manual §5.2.10)

#### Scenario: Each site has a non-empty cell covering its tetrahedra
- GIVEN a Delaunay tetrahedralization
- WHEN the Voronoi diagram is built
- THEN each input vertex incident to at least one tetrahedron has a non-empty cell,
  and the cells' tetrahedron indices together cover every tetrahedron

### Requirement: Robust to degenerate tetrahedra

CyberMeshGenerator SHALL guard the circumcenter computation against
near-degenerate (near-zero-volume) tetrahedra and SHALL NOT produce non-finite
Voronoi vertices; dual edges of a skipped degenerate tetrahedron MAY be omitted.

#### Scenario: No non-finite vertices
- GIVEN a tetrahedralization that may contain a near-flat tetrahedron
- WHEN the Voronoi diagram is built
- THEN every emitted Voronoi vertex has finite coordinates
