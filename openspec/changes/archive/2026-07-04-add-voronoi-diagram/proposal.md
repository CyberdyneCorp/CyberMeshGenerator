# Add Voronoi diagram (Phase 9)

## Why

The project is a "Delaunay-based quality tetrahedral mesh generator **and 3D
Delaunay/Voronoi engine**", but only the Delaunay half exists. The Voronoi diagram
is the geometric dual of the Delaunay tetrahedralization and a headline TetGen
feature (`-v`). It is the cleanest capability left to add: it derives entirely from
the existing DT and circumcenters, with no boundary-recovery dependency.

## Duality

For a Delaunay tetrahedralization of a point set, the Voronoi dual is exact:

| Delaunay | Voronoi |
|----------|---------|
| tetrahedron | vertex (its circumcenter) |
| triangular face (2 tets) | edge (between the two circumcenters) |
| convex-hull face (1 tet) | ray (from the circumcenter, outward) |
| input vertex | cell (the circumcenters of its incident tetrahedra) |

A Voronoi vertex is equidistant from the four vertices of its Delaunay tetrahedron
(the circumcenter property), and by the empty-circumsphere property no input point
is closer — which makes the construction directly checkable.

## What changes

Spec delta for the **voronoi-diagram** capability:

- `cmg::voronoi::VoronoiDiagram` and `cmg::voronoi::build(const Mesh&)`:
  - **vertices** — one circumcenter per Delaunay tetrahedron;
  - **edges** — one per Delaunay face: a finite segment between the two adjacent
    tetrahedra's circumcenters, or a ray (marked `v1 = -1`) with an outward unit
    direction for a convex-hull face;
  - **cells** — per input vertex, the set of incident Voronoi vertices (the
    circumcenters of the tetrahedra around that vertex).
- Operates on the raw Delaunay mesh of a point set (as TetGen's `-v` does).

## Impact

- `voronoi(delaunay(points))` yields the Voronoi diagram; validated by the
  circumcenter-equidistance and empty-sphere duality properties, and by
  Euler/duality counts (one Voronoi vertex per tet, one edge per face).
- Foundation for the power (weighted) diagram — the same construction with
  orthocenters — noted below.

## Non-goals (deferred)

- **Voronoi facets** (the polygons dual to Delaunay edges) — the vertex/edge/cell
  structure lands here; the ordered facet rings around each Delaunay edge are a
  follow-up.
- **Power / weighted (regular) diagram** (`-vw`) — the same construction using
  orthocenters instead of circumcenters; deferred until the weighted DT path is
  exercised here.
- **Voronoi file output** (`.v.node`/`.v.edge`/`.v.face`/`.v.cell`) — a small
  file-formats follow-up; this change delivers the in-memory diagram.
- **Clipping the diagram to a bounded box** — rays are returned unbounded with a
  direction, matching TetGen.
