# Add mesh quality metrics and reporting

## Why

Quality mesh generation (`-q`) has two halves: *measuring* element quality and
*refining* to improve it. This change delivers the measurement half — the
radius-edge / dihedral-angle / volume statistics TetGen prints (`-V`) — which is
robust, useful on its own (diagnosing a mesh, driving downstream decisions), and a
genuine part of the quality-mesh-generation capability.

## Why shape refinement is NOT in this change (with evidence)

The other half — *refining* to bound the radius-edge ratio or minimum dihedral —
was empirically shown to **diverge** with straightforward Delaunay refinement:
inserting circumcenters of over-ratio tetrahedra (batch re-triangulation, pure
circumcenters, even with a near-duplicate guard) blows up to thousands of Steiner
points and never converges at bounds 2.0, 2.5, or 3.0. Robust 3D shape refinement
needs incremental single-point insertion with a bad-tetrahedron queue,
encroachment protection, and sliver handling (the reason TetGen/CGAL devote
thousands of lines to it). It is therefore deferred as a dedicated effort, not
attempted here.

## What changes

Spec delta for the **quality-mesh-generation** capability:

- `cmg::quality::report(mesh, worst_count)` → `QualityReport`: per-mesh min/max/mean
  radius-edge ratio, min/max/mean dihedral angle (degrees), min/max/total volume, a
  dihedral-angle histogram, and the indices of the worst tetrahedra by radius-edge
  ratio. Degenerate tets are handled without NaNs.

## Impact

- A mesh's shape quality is measurable: `report(mesh).min_dihedral`,
  `.max_radius_edge`, the histogram, and the worst-element list support diagnostics
  and comparing before/after smoothing or refinement.

## Non-goals
- **Radius-edge / dihedral refinement** (shape improvement by Steiner insertion) —
  deferred with the divergence evidence above.
- **Anisotropic quality measures** — isotropic scalar metrics only.
