# Add quality mesh generation (Phase 4, increment 1)

## Why

Phases 1–3 produce a valid mesh, but its element *sizes* and *shapes* are dictated
by the input vertices alone — tetrahedra can be arbitrarily large or badly shaped.
Finite-element and finite-volume solvers need bounded element size and shape. This
change adds Delaunay refinement: inserting Steiner points to bound tetrahedron
shape (radius-edge ratio) and size (maximum volume), the `-q` / `-a` family.

## Scope of this increment

Full TetGen quality meshing is large. It splits cleanly into a **size** bound
(maximum volume) and a **shape** bound (radius-edge ratio / minimum dihedral). This
increment delivers the size bound, which is robust and terminating, and **defers
the shape bound**, which is not — see below.

**Delaunay refinement by Steiner-point insertion to enforce a maximum tetrahedron
volume** (`MeshOptions::max_volume`), bounded by a **Steiner-point budget**
(`MeshOptions::steiner_budget`). Each round re-meshes with the Phase 1–3 kernels,
finds tetrahedra over the volume bound, and inserts their circumcenter (when inside
the domain) or centroid (a boundary-safe fallback). Volume refinement is
**self-limiting**: subdivision strictly reduces tetrahedron volume toward the bound,
and slivers (tiny volume) are never targeted, so it terminates and conserves the
domain volume.

### Why radius-edge (shape) refinement is deferred, not shipped

Enforcing a radius-edge bound by inserting circumcenters of "skinny" tetrahedra
**diverges** without sliver-exudation and boundary-encroachment handling: a nearly
flat sliver has an enormous radius-edge ratio, its circumcenter insertion creates
more slivers, and the mesh explodes (empirically ~13k tetrahedra with the ratio
*increasing*, never meeting the bound). Shipping that would be a regression, so this
increment refines on volume only and defers shape refinement to a later increment
that does it correctly (circumcenter refinement + sliver removal + encroachment
protection).

## What changes

Spec delta for the **quality-mesh-generation** capability:

- `cmg::quality::refine(points, plc?, opts)`: a Delaunay-refinement loop that
  re-meshes with the Phase 1–3 kernels each round, finds tetrahedra over the volume
  bound, and inserts a Steiner point per over-large tetrahedron until the bound is
  met or the Steiner budget is exhausted.
- `delaunay()` and `tetrahedralize()` invoke refinement when `max_volume` is set.

## Impact

- `tetrahedralize(cube, {.plc=true, .max_volume=0.05})` returns a mesh whose every
  tetrahedron has volume ≤ 0.05 while still conserving the domain volume.

## Non-goals (deferred to later quality increments)

- **Radius-edge / shape refinement** and **minimum dihedral angle / sliver removal**
  — the shape bound needs sliver exudation + boundary-encroachment protection to
  converge; deferred (see the scope rationale above). `MeshOptions::quality` is
  accepted but does not yet drive shape refinement.
- **Boundary encroachment protection** and provable-termination boundary handling
  for non-convex PLCs — this increment uses a centroid fallback for boundary
  tetrahedra rather than splitting encroached boundary faces.
- **Per-region and per-facet/segment size constraints** (`.var`, region max-volume)
  — tied to region-attributes (Phase 6) and sizing (Phase 5).
- **Background-mesh sizing function** (`-m`) — Phase 5.
- **Single-precision (`CMG_SINGLE`) volume-conservation under refinement**: the
  ray-cast carve loses fidelity when refinement packs many tetrahedra against the
  boundary in `float` (measured ~20% volume loss on the refined cube), mirroring
  TetGen's own `-DSINGLE` lower-quality caveat. Domain-volume conservation is a
  **double-precision guarantee** this increment; robust single-precision carving
  waits on boundary recovery. (Predicate signs and the DT itself remain robust in
  `float`; only the ray-cast interior classification degrades.)
