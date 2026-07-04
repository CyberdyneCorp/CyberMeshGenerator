# Add adaptive mesh sizing (Phase 5, increment 1)

## Why

Phase 4 refines to a single global maximum volume — uniform everywhere. Real
meshes need to be fine where the geometry or physics demands it and coarse
elsewhere. TetGen's `-m` drives refinement from a **sizing function**: a
spatially-varying target element size. This change adds that, reusing the Phase 4
self-limiting volume-refinement engine.

## Approach

A sizing function gives, at any point, the desired local edge length `h`. We map it
to a **spatially-varying volume target** — `h(p)³ / (6√2)`, the volume of a regular
tetrahedron of edge `h` — and refine each tetrahedron against the target evaluated
at its centroid. Because the criterion is volume-based, it inherits Phase 4's
robustness: it is self-limiting (slivers have small volume and are not
over-refined) and terminating, and it avoids the sliver-divergence that sinks a
naive edge-length criterion.

## What changes

Spec delta for the **adaptive-mesh-sizing** capability:

- **Analytic sizing callback** — `MeshOptions::sizing` is an optional
  `std::function<const Point3& -> double>` returning the target edge length at a
  point (≤ 0 means "no constraint here", matching TetGen's size-0 convention). This
  is the idiomatic C++ form of TetGen's `tetunsuitable` programmatic callback.
- **Background-mesh sizing** — `cmg::sizing::from_background(bg_mesh, node_sizes,
  scale)` builds such a sizing function from a background tetrahedral mesh with a
  per-node size, locating each query point in the background mesh and
  barycentrically interpolating the size (TetGen's `-m` background-mesh / `.mtr`
  data, at the API level). A `scale` factor implements `-m#` metric scaling.
- Refinement honors the sizing function (alone or combined with a global
  `max_volume`, taking the tighter target per tetrahedron); `tetrahedralize()` /
  `delaunay()` refine when `sizing` or `max_volume` is set.

## Impact

- `tetrahedralize(box, {.plc=true, .sizing=[](Point3 p){ return p.x < 0.5 ? 0.15 : 0.6; }})`
  yields a mesh that is fine in the `x < 0.5` half and coarse elsewhere.
- A background mesh carrying node sizes drives interpolated, smoothly-graded
  refinement.

## Non-goals (deferred)

- **`.mtr` file parsing** — the in-memory sizing API and background-mesh path land
  here; reading TetGen `.mtr`/`.b.*` files is a small follow-up on the file-formats
  layer.
- **Anisotropic / tensor metrics** — this increment is isotropic (a scalar size
  per point), as is TetGen's default `.mtr`.
- **Gradient-limiting / smoothness control** of the size field beyond what the
  supplied function provides.
- **Single-precision volume fidelity under sizing-driven refinement** — same
  double-precision guarantee and `CMG_SINGLE` caveat as Phase 4's carve.
