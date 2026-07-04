# Add Voronoi file output and power (weighted) diagram

## Why

Phase 9 built the Voronoi diagram in memory but deferred two items: writing it to
TetGen's `.v.*` files, and the power (weighted / Laguerre) diagram dual to the
weighted Delaunay tetrahedralization. Both are natural extensions of the existing
construction and complete the Voronoi feature.

## What changes

Spec delta for the **voronoi-diagram** capability:

- `cmg::io::write_voronoi(base, diagram, index_base)` — writes `.v.node` (Voronoi
  vertices, `.node` format), `.v.edge` (edges as `<e#> <v1> <v2> <Vx> <Vy> <Vz>`
  where `v2 = -1` marks a ray with unit direction `(Vx,Vy,Vz)`), and `.v.cell`
  (per-site incident-vertex lists), following TetGen's `-v` output.
- `cmg::voronoi::build_power(mesh, weights)` — the power diagram: the same dual
  construction using each tetrahedron's **orthocenter** (weighted circumcenter)
  instead of its circumcenter, for the weighted-Delaunay mesh of weighted points.

## Impact

- `voronoi(delaunay(points))` can be serialized to the TetGen `.v.*` files and
  re-read by TetGen-aware tools.
- Weighted point sets get their power diagram, matching TetGen's `-vw`.

## Non-goals

- **Voronoi facets** (the polygons dual to Delaunay edges, `.v.face` full ordered
  rings) — still deferred; `.v.face` is not written this change.
- **Reading** `.v.*` files back — output only (they are a terminal visualization
  format).
- **Clipping** rays to a bounded box — rays are written unbounded with a direction.
