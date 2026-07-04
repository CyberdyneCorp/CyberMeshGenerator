# Design — Delaunay tetrahedralization

## Context

This is the DT kernel the foundation deferred. It must be **robust** (correct on
degenerate input, using the exact predicates), **incremental** (Bowyer-Watson, so
constrained/quality phases can reuse point insertion later), and produce a result
**equal to TetGen's** in general position.

## Algorithm — incremental Bowyer-Watson

State is a set of tetrahedra with explicit face-adjacency, held in a per-call
context (no globals). Each `Tet` stores four vertex indices and four neighbor tet
indices (neighbor across the face *opposite* vertex k), plus a `dead` flag.

1. **Bounding simplex.** Compute the input bounding box and add four
   super-vertices forming a tetrahedron that strictly encloses all input points
   (scaled well beyond the box). Start from this single super-tet. Super-vertices
   are appended after the real points; tets touching them are removed at the end.
2. **Insertion order.** Sort input point indices with BRIO-Hilbert (below), then
   insert one at a time.
3. **Point location.** Walk from the most-recently-created tet toward the point:
   at each tet, if the point is on the negative side of a face (`orient3d < 0`),
   step to that face's neighbor; when no face rejects it, the point is inside.
   The walk uses exact `orient3d`, so it terminates and lands in the containing
   tet even on coplanar faces.
4. **Cavity (Delaunay).** Flood-fill from the containing tet across neighbors,
   collecting every tet whose circumsphere contains the point (`insphere > 0`).
   With exact predicates the cavity is connected and star-shaped w.r.t. the point.
5. **Re-triangulation.** The cavity's boundary is the set of faces shared between a
   cavity tet and a non-cavity tet (or the hull). Delete the cavity tets; for each
   boundary face create a new tet joining the face to the point. Re-link
   adjacency: across the boundary face to the external neighbor; across side faces
   to sibling new tets, matched by shared edge via an edge→(tet,slot) map.
6. **Finalize.** Remove every tet incident to a super-vertex. The survivors are the
   DT of the input points; their boundary faces (neighbor removed) are the convex-
   hull faces, emitted with marker 1. Optionally emit the neighbor array.

Orientation convention matches the foundation's single-tet case: stored tets keep
`orient3d(v0,v1,v2,v3) < 0`; new tets are created with a consistent sign so the
neighbor bookkeeping and later reuse stay coherent.

## BRIO-Hilbert spatial sort

- **Hilbert order**: map each point to its index along a 3-D Hilbert curve at a
  chosen order (bit depth) over the bounding box; sorting by that key gives strong
  spatial locality so the point-location walk is short.
- **BRIO**: partition points into O(log n) rounds of geometrically growing size
  (each point promoted to the next round with fixed probability); Hilbert-sort
  within a round. This "biased randomized insertion order" keeps the expected
  running time near-linear while avoiding worst-case walks.
- **Control**: exposed through the same knobs TetGen's `-b` provides —
  threshold / ratio / Hilbert order — with sorting fully disableable (insert in
  input order). Deterministic given a fixed seed (via NumPP's bit-exact `random`
  when integrated, else a small internal PRNG), so runs are reproducible.

## Weighted (regular) Delaunay

Under `MeshOptions::weighted`, each point's weight lifts it to
`(x, y, z, x²+y²+z²−w)`; the in-sphere test becomes the power test, evaluated with
the ported `orient4d` predicate. Dominated points fail every insertion cavity test
and end up in no tetrahedron — retained in `Mesh::points` but absent from
`Mesh::tetrahedra`, matching TetGen. `weighted` remains mutually exclusive with a
PLC/reconstruct (already enforced by the switch parser).

## Robustness

- All orientation/in-sphere/power decisions go through the exact predicates; no
  `>`/`<` on raw floating-point determinants.
- The super-tet coordinates are chosen large relative to the input extent so hull
  insertions are decided correctly; tests assert the empty-circumsphere property
  on the final mesh to catch any hull/degeneracy error.
- Duplicate points are detected (coincident within the coplanar tolerance) and
  collapsed to the first occurrence, mirroring TetGen's degeneracy handling.

## Acceleration hooks

The Hilbert-key computation, the point-location walk's batched candidate tests,
and the cavity in-sphere scan are expressed so they can call the
backend-dispatch shim (`backend::should_offload`). On this change they run the CPU
path; the device kernels are Phase 11. This keeps the deterministic-topology
contract: the GPU only filters candidates, the topological mutation stays on the
exact CPU path.

## Testing

- **Invariants**: every output tet has positive volume and an empty circumsphere
  (no other input vertex strictly inside), and the tet/face/vertex counts satisfy
  the Euler relation for a convex triangulation.
- **Known cases**: cube-corner sets, points on a sphere (cospherical degeneracy),
  a random cloud (empty-sphere check over all vertices), and a duplicate-point
  input.
- **Oracle**: compare against TetGen for general-position clouds (sorted-tuple
  equality) and assert validity + hull equality for degenerate ones, per the
  tetgen-oracle conventions.
