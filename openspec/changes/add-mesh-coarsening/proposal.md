# Add mesh coarsening (Phase 8)

## Why

TetGen `-R` coarsens a mesh by removing vertices. Coarsening reduces element count
where the mesh is over-refined. This increment delivers vertex-removal coarsening
that preserves the domain.

## What changes

Spec delta for the **mesh-coarsening** capability:

- `cmg::coarsen::coarsen(mesh, opts)` — remove a deterministic fraction of the
  **interior** vertices (never boundary vertices) and re-tetrahedralize the
  remaining vertices, returning a coarser valid mesh. `keep_fraction` controls how
  many interior vertices are kept.

## Impact

- An over-refined mesh can be thinned: `coarsen(mesh, {.keep_fraction = 0.3})`
  yields far fewer tetrahedra while keeping the boundary — and hence the domain —
  intact.

## Non-goals
- **Quality-driven / error-driven vertex selection** — this increment removes a
  random (seeded) fraction of interior vertices, not the least-important ones.
- **Boundary-vertex decimation** (coarsening the surface) — boundary vertices are
  always kept.
- **Guaranteed element-count target** — `keep_fraction` is approximate.
