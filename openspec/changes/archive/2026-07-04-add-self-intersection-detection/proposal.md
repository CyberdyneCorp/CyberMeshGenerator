# Add PLC self-intersection detection (TetGen -d)

## Why

Constrained tetrahedralization assumes a valid (non-self-intersecting) PLC. TetGen's
`-d` detects self-intersections so a bad input is caught before meshing. This is a
robust, exact-predicate geometric test and a genuine part of the
constrained-tetrahedralization capability — and it composes with the existing PLC
pipeline as an input-validation step.

## Why facet-interior recovery is NOT in this change

The remaining hard piece of constrained tetrahedralization — **facet (triangle
interior) recovery**, forcing PLC facets to appear as mesh faces — needs a per-facet
2-D Delaunay triangulation plus conforming (Ruppert-style) refinement through
coplanar-facet degeneracies. That is a dedicated multi-component effort (segment
recovery, delivered earlier, is its prerequisite) and is deferred, not attempted
here.

## What changes

Spec delta for the **constrained-tetrahedralization** capability:

- `cmg::detect::self_intersections(plc)` → the list of intersecting facet-triangle
  pairs, computed by exact-predicate triangle-triangle intersection over all
  non-adjacent facet triangles (each facet polygon fan-triangulated). Triangles that
  merely share a vertex or edge are not reported.

## Impact

- A caller can validate a PLC (e.g. one loaded from STL) before meshing:
  `if (!detect::self_intersections(plc).empty()) { ... }`.

## Non-goals
- **Facet-interior recovery** and true CDT facet conformance — deferred (see above).
- **Repairing** intersections — detection only.
- **Segment/facet coplanar-overlap classification** beyond crossing intersection.
