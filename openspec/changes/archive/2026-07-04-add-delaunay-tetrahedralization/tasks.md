# Tasks — Add Delaunay tetrahedralization (Phase 1)

## Kernel data structures
- [x] `Tet` record (4 vertex indices, 4 neighbor indices, dead flag) + per-call `Triangulation` context (no globals)
- [x] Bounding super-tetrahedron construction (super-vertices appended after real points)
- [x] Face/edge helpers: opposite-vertex face slots, sorted-edge and sorted-face keys

## Incremental Bowyer-Watson
- [x] Point location by exact `orient3d` walk from the last-created tet
- [x] Delaunay cavity flood-fill by exact `insphere`
- [x] Cavity boundary extraction + re-triangulation joining the point to each boundary face
- [x] Adjacency re-linking (external neighbor across base face; sibling new tets via edge→slot map)
- [x] Finalize: drop super-vertex tets; extract survivors as the DT

## BRIO-Hilbert sort
- [x] 3-D Hilbert-curve key computation over the bounding box
- [x] BRIO rounds (geometric growth, fixed promotion probability), Hilbert-sorted within a round
- [~] Enable/disable + deterministic seed exposed (`spatial_sort`, `sort_seed`); full TetGen `-b` threshold/ratio/order knobs deferred
- [x] Route Hilbert-key computation through `backend::should_offload` (CPU path active)

## Weighted (regular) DT
- [x] Point weights + lifted power test via ported `orient4d`
- [x] Dominated points retained in `Mesh::points`, absent from `Mesh::tetrahedra`
- [x] Reject weighted + PLC (already enforced by the switch parser; assert in kernel)

## Output
- [x] Convex-hull faces emitted in `Mesh::faces` with marker 1
- [x] Optional `Mesh::neighbors` under `emit_neighbors`
- [~] Duplicate-point collapse (exact coincidence implemented; within-tolerance merge deferred)
- [x] `delaunay()` routes n>4 to the kernel; remove the foundation `NotImplemented` stub

## Tests & oracle
- [x] Empty-circumsphere invariant over all vertices; positive volume; Euler relation
- [x] Known cases: cube corners, cospherical set, random cloud, duplicates, coplanar-input error
- [x] Determinism (fixed seed) and sort-on/off validity equivalence
- [~] TetGen oracle: sorted-tuple equality (general position) + hull/validity (degenerate)  (invariant-validated now; live TetGen diff lands with file-formats in Phase 2)
- [x] `openspec validate --all --strict` green; foundation + Phase 1 tests pass CPU-only
