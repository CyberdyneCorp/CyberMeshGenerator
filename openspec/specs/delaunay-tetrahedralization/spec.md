# delaunay-tetrahedralization Specification

## Purpose
TBD - created by archiving change add-delaunay-tetrahedralization. Update Purpose after archive.
## Requirements
### Requirement: Delaunay tetrahedralization of a point set

CyberMeshGenerator SHALL compute the Delaunay tetrahedralization of a 3-D point set
(n ≥ 4 non-coplanar points) via incremental Bowyer-Watson insertion, returning a
`Mesh` whose every tetrahedron has positive volume and satisfies the
empty-circumsphere property: no input vertex lies strictly inside the circumsphere
of any output tetrahedron. All orientation and in-sphere decisions SHALL use the
exact `orient3d` / `insphere` predicates. (oracle: TetGen delaunay-tetrahedralization;
tetgen.cxx incremental Bowyer-Watson insertion)

#### Scenario: Delaunay of a random cloud satisfies the empty-sphere property
- GIVEN a set of points in general position
- WHEN `delaunay(points, {})` is called
- THEN it returns a Mesh in which, for every tetrahedron, no other input vertex is
  strictly inside its circumsphere

#### Scenario: Result matches TetGen in general position
- GIVEN a general-position point set fed to both tools
- WHEN each computes the Delaunay tetrahedralization
- THEN the sets of tetrahedra match as sorted vertex-index tuples

#### Scenario: Degenerate points produce a valid tetrahedralization
- GIVEN a point set containing cospherical or coplanar subsets
- WHEN `delaunay` is called
- THEN degeneracies are resolved via the exact predicates and the result is a
  valid, gap-free tetrahedralization of the convex hull

#### Scenario: Duplicate points are collapsed
- GIVEN a point set with coincident points (within the coplanar tolerance)
- WHEN `delaunay` is called
- THEN duplicates are collapsed to their first occurrence and the tetrahedralization
  is valid

### Requirement: BRIO-Hilbert spatial sorting

CyberMeshGenerator SHALL order incremental point insertion using a biased
randomized insertion order (BRIO) over rounds sorted by a 3-D Hilbert-curve key, to
give insertion locality and near-linear expected performance. The sort SHALL be
deterministic for a fixed seed and SHALL be fully disableable, in which case points
are inserted in input order. Disabling or changing the sort SHALL NOT change the
empty-circumsphere validity of the result. (oracle: tetgen.cxx BRIO-Hilbert sort
3272-3338)

#### Scenario: Sorting improves locality but not correctness
- GIVEN the same point set
- WHEN it is tetrahedralized once with BRIO-Hilbert sorting on and once off
- THEN both results satisfy the empty-circumsphere property and triangulate the
  same convex hull

#### Scenario: Deterministic given a seed
- GIVEN a fixed sort seed
- WHEN the same point set is tetrahedralized twice
- THEN the insertion order and the resulting mesh are identical between runs

### Requirement: Weighted (regular) Delaunay tetrahedralization

Under `MeshOptions::weighted`, CyberMeshGenerator SHALL compute the weighted
(regular) Delaunay tetrahedralization, where each point's weight lifts it to
`(x, y, z, x²+y²+z²−w)` and the in-sphere test is replaced by the power test
evaluated with the exact `orient4d` predicate. Dominated (redundant) points SHALL
be retained in `Mesh::points` but SHALL belong to no tetrahedron. Weighted mode
SHALL NOT be combined with a PLC or reconstruction. (oracle: tetgen.cxx:3266, 3712)

#### Scenario: Regular triangulation with redundant points
- GIVEN weighted points where some are dominated by others' power
- WHEN `delaunay(points, {.weighted = true})` is called
- THEN the regular triangulation is produced and each dominated point appears in
  `Mesh::points` but in no tetrahedron

#### Scenario: Weighted excludes PLC
- WHEN weighted mode is requested together with a PLC
- THEN the call is rejected with an error rather than producing a mesh

### Requirement: Convex-hull face output

For a point-set Delaunay tetrahedralization, CyberMeshGenerator SHALL populate
`Mesh::faces` with the faces of the convex hull of the point set — each boundary
face (whose tetrahedron has no neighbor across it) carrying boundary marker 1.
(oracle: TetGen delaunay-tetrahedralization, manual §4.2.1 Table 3)

#### Scenario: Hull faces are emitted and marked
- WHEN a point-set DT is computed
- THEN `Mesh::faces` contains exactly the convex-hull triangles, each with
  `face_markers` value 1, and interior faces are not emitted

### Requirement: Optional neighbor adjacency output

When `MeshOptions::emit_neighbors` is set, CyberMeshGenerator SHALL populate
`Mesh::neighbors` with four entries per tetrahedron giving the index of the
tetrahedron adjacent across each face, or −1 where the face is on the convex hull.
(oracle: tetgen.h neighborlist; TetGen -n)

#### Scenario: Neighbor indices are consistent
- WHEN `delaunay(points, {.emit_neighbors = true})` is called
- THEN each tetrahedron has four neighbor entries, hull faces read −1, and every
  non-hull neighbor relation is symmetric (if A lists B, B lists A)

### Requirement: Recoverable failure on insufficient or degenerate input

CyberMeshGenerator SHALL return a `MeshError` (never terminate) when a Delaunay
tetrahedralization cannot be formed: fewer than four points, or all points
coplanar. (oracle: mesh-core-foundation error model)

#### Scenario: All points coplanar
- GIVEN four or more points that are all coplanar
- WHEN `delaunay` is called
- THEN it returns a `MeshError` with code `InvalidInput`, without throwing

