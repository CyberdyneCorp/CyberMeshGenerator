# voronoi-diagram Specification (output + power delta)

## ADDED Requirements

### Requirement: Voronoi file output

CyberMeshGenerator SHALL write a Voronoi diagram to TetGen's `.v.node` and `.v.edge`
files (and `.v.cell`), where `.v.node` lists the Voronoi vertices in `.node` format,
`.v.edge` lists each edge as `<e#> <v1> <v2> <Vx> <Vy> <Vz>` with `v2 = -1` denoting a
ray whose unit direction is `(Vx,Vy,Vz)`, and `.v.cell` lists each site's incident
Voronoi vertices. Objects SHALL be numbered from the given index base. (oracle: TetGen
voronoi-diagram output; manual §5.2.10)

#### Scenario: Voronoi vertices and edges written
- GIVEN a Voronoi diagram
- WHEN `write_voronoi(base, diagram)` is called
- THEN `base.v.node` contains all Voronoi vertices and `base.v.edge` contains every
  edge, finite edges giving two vertex indices and rays giving `v2 = -1` with a unit
  direction

### Requirement: Power (weighted) diagram

CyberMeshGenerator SHALL provide `cmg::voronoi::build_power(mesh, weights)` returning
the power diagram of a weighted-Delaunay mesh, using each tetrahedron's orthocenter
(weighted circumcenter) as its Voronoi vertex; the edge and cell duality SHALL match
the unweighted construction. (oracle: TetGen power diagram, `-vw`; tetgen.cxx)

#### Scenario: Power vertices are orthocenters
- GIVEN a weighted point set, its weighted-Delaunay mesh, and the weights
- WHEN the power diagram is built
- THEN it has one vertex per tetrahedron (the orthocenter) and one edge per face,
  with rays for hull faces, matching the Voronoi structure
