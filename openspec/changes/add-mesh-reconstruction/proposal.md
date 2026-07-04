# Add mesh reconstruction (Phase 9)

## Why

TetGen `-r` reads an existing mesh and refines it. With Phase 2 I/O and Phase 4/5
refinement in place, reconstruction is the small glue that ties them together:
re-tetrahedralize an existing mesh's vertices under new constraints.

## What changes

Spec delta for the **mesh-reconstruction** capability:

- `cmg::reconstruct::reconstruct(mesh, opts)` — re-tetrahedralize the input mesh's
  vertex set and apply the refinement/sizing in `opts` (a new `max_volume`,
  `sizing`, etc.), returning the refined mesh. The vertex set (and the convex domain
  it spans) is preserved.

## Impact

- A mesh loaded from `.ele`/`.node` (Phase 2) can be refined to finer constraints:
  `reconstruct(read_mesh("m.1.ele"), {.max_volume = 0.01})`.

## Non-goals
- **Preserving the exact input connectivity** while refining (true in-place
  refinement) — this increment re-tetrahedralizes the vertex set (a superset-
  preserving Delaunay refine), not an incremental in-place refinement.
- **`.vol`-file-driven per-tet refinement** — the `.vol` reader exists (constraint
  files); wiring per-tet volume targets into reconstruction is deferred.
