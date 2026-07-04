# Add constrained/boundary-conforming tetrahedralization (Phase 3, increment 1)

## Why

`tetrahedralize(PLC)` currently returns `NotImplemented` for a faceted PLC. To mesh
an actual solid — the point of the library — we must tetrahedralize the *interior*
of a domain bounded by a Piecewise-Linear Complex, not just the convex point set.
This change delivers the first increment of that capability.

## Scope of this increment

Robust, exact 3D **constrained Delaunay with facet preservation** (boundary
recovery, Schönhardt handling, minimal Steiner points, `-Y`) is a large,
multi-change effort. This increment delivers the correct and useful core:

**Boundary-conforming tetrahedralization of a PLC domain** =
Delaunay tetrahedralization of the PLC vertices, then **interior classification**
that keeps only the tetrahedra inside the domain (ray-casting each tet centroid
against the PLC boundary facets), and emits the domain boundary faces.

- **Exact for convex (and star-shaped) domains**: the DT of the vertices already
  tetrahedralizes such a domain and its boundary is respected (the hull faces lie
  on the domain boundary, re-diagonalized — which is exactly TetGen's default `-p`
  behavior, as opposed to `-Y` exact-facet preservation). Volume is conserved.
- **Facet-bounded holes/voids** are removed by the same ray-cast test.
- Boundary faces of the kept mesh are emitted with marker 1.

## What changes

Spec delta for the **constrained-tetrahedralization** capability:

- A `cmg::cdt::tetrahedralize_plc(PLC, MeshOptions)` pipeline: facet triangulation
  → DT of PLC vertices → ray-cast interior carve → boundary-face extraction.
- `tetrahedralize(PLC, opts)` routes a faceted PLC here (removing the
  `NotImplemented` stub); a point-only PLC still routes to `delaunay()`.

## Impact

- Solids can be meshed: a cube/tetrahedron/convex polyhedron PLC (including one
  loaded from `.smesh`/`.off`/STL via Phase 2) yields a valid interior tetrahedral
  mesh with a conforming boundary and conserved volume.
- Validated against TetGen: for a convex domain the kept-tet volume equals the
  domain volume and the boundary faces lie on the PLC surface.

## Non-goals (deferred to later constrained-tetrahedralization increments)

- **Exact facet preservation (`-Y`)** and true constrained-Delaunay boundary
  recovery (segment/facet recovery by flips + Steiner points). This increment
  re-diagonalizes coplanar boundary faces rather than preserving input diagonals.
- **Exact non-convex boundary conformance**: where the domain is concave, DT tets
  can straddle the boundary; interior classification is by centroid, so concave
  boundaries are approximated until facet recovery lands. Convex/star-shaped
  domains are exact.
- **Seed-only holes/regions** without bounding facets, and **region attribute**
  assignment — deferred to Phase 6 (`region-attributes`).
- **Non-convex (non-fan-triangulable) facet polygons** — facets are fan-triangulated
  this increment; general polygon triangulation is deferred.
- Quality refinement / Steiner sizing — Phase 4.
