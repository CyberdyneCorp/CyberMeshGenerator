# Add mesh optimization (Phase 7, increment 1)

## Why

The meshes produced so far are valid but not shape-optimized — interior vertex
positions are wherever insertion/refinement left them. Laplacian smoothing improves
element shape (dihedral angles) cheaply and safely by relocating interior vertices,
without changing the domain or the boundary. This is the first increment of
mesh-optimization (TetGen `-O`).

## What changes

Spec delta for the **mesh-optimization** capability:

- `cmg::optimize::laplacian_smooth(mesh, opts)`: relocate each interior vertex (one
  not on any boundary face) toward the centroid of its edge-neighbors, over a
  configurable number of sweeps, **never inverting an incident tetrahedron** (a move
  that would flip any incident tet's orientation is damped or skipped). Boundary
  vertices are fixed.
- `cmg::optimize::min_dihedral_angle(mesh)`: the minimum dihedral angle over all
  tetrahedra — a standard shape-quality measure used to demonstrate improvement.

## Impact

- Smoothing a refined mesh improves (or does not worsen) its minimum dihedral angle,
  introduces no inverted tetrahedra, and — because only interior vertices move and
  the boundary is fixed — conserves the domain volume exactly.

## Non-goals (deferred to later optimization increments)

- **Topological transformations** (edge/face flips, 2-3 / 3-2 flips) — this increment
  is vertex-relocation only.
- **Sliver-targeted optimization** and **optimal-point (smart Laplacian / ODT / CVT)
  smoothing** — plain Laplacian toward the neighbor centroid this increment.
- **Boundary-vertex smoothing** along facets/segments — boundary vertices stay fixed.
- **Second-order (`-o2`) node insertion** — separate.
