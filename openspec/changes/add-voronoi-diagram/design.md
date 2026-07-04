# Design — Voronoi diagram

## Data model

```cpp
namespace cmg::voronoi {

struct Edge {
    int v0;        // index into vertices
    int v1;        // index into vertices, or -1 for a ray
    Point3 dir;    // outward unit direction when v1 == -1 (else unused)
};

struct VoronoiDiagram {
    std::vector<Point3> vertices;           // circumcenter per Delaunay tet
    std::vector<Edge> edges;                // dual to Delaunay faces
    std::vector<std::vector<int>> cells;    // per input vertex -> incident tets
};

VoronoiDiagram build(const Mesh& delaunay);

} // namespace cmg::voronoi
```

## Construction

Input is a Delaunay `Mesh` (tets stored `orient3d(v0..v3) < 0`).

1. **Vertices** — for each tetrahedron `t`, compute its circumcenter (the point
   equidistant from its four vertices); `vertices[t] = circumcenter(t)`. A
   degenerate (near-zero-volume) tetrahedron is guarded; its circumcenter is
   flagged and its dual edges are skipped.

2. **Edges** — hash each tetrahedron face (sorted vertex triple) to the tets that
   own it:
   - a face shared by tets `i` and `j` (interior) → `Edge{i, j, -}`;
   - a face owned by a single tet `i` (convex hull) → `Edge{i, -1, n}` where `n`
     is the outward unit normal of that face (pointing away from the tet's fourth
     vertex). This is the Voronoi ray to infinity.

3. **Cells** — for each input vertex `p`, gather the indices of the tetrahedra
   incident to `p`; those circumcenters are `cells[p]`. (A full cell also needs the
   ordered facet boundary; the unordered incident-vertex set is what this increment
   provides — enough for the site's Voronoi neighborhood and the duality checks.)

## Circumcenter

Reuses the formula already used by refinement: for `a,b,c,d` with `B=b−a`,
`C=c−a`, `D=d−a`,

```
cc = a + ( |B|²(C×D) + |C|²(D×B) + |D|²(B×C) ) / ( 2 · B·(C×D) )
```

Extracted into a small shared geometry helper so refinement and Voronoi share one
implementation.

## Correctness checks (tests)

- **Counts**: `vertices.size() == tets` ; number of finite edges == interior faces,
  number of rays == hull faces.
- **Circumcenter property**: each Voronoi vertex is equidistant (within tolerance)
  from the four vertices of its Delaunay tetrahedron.
- **Empty-sphere duality**: no input point lies strictly inside the circumsphere of
  any tetrahedron (already a DT invariant) — so each Voronoi vertex's nearest input
  sites are exactly its tetrahedron's four vertices.
- **Ray direction**: each hull-face ray points outward (the tetrahedron's fourth
  vertex is on the opposite side of the face from the direction).
- **Cell coverage**: every input vertex has a non-empty cell, and the union of all
  cells' tet indices covers all tets.

## Why not carve

The Voronoi diagram is the dual of the *unrestricted* Delaunay tetrahedralization of
the point set (TetGen's `-v` operates on the DT), so `build` takes the raw
`delaunay(points)` mesh, not a PLC-carved domain. Passing a carved mesh still
produces a valid dual of that tet set, but the canonical Voronoi diagram uses the
full DT.
