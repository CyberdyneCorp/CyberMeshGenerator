# Design — boundary-conforming tetrahedralization

## Why this shape

A full 3D constrained Delaunay tetrahedralization needs boundary recovery: forcing
the PLC's segments and facets to appear in the mesh, inserting Steiner points where
they cannot (Schönhardt polyhedra). That is genuinely hard and spans multiple
changes. But a large and useful class — **convex and star-shaped domains** — needs
none of it: the Delaunay tetrahedralization of the boundary vertices already fills
the domain, and its boundary faces lie on the domain surface (re-diagonalized).
This increment delivers that correctly and defers recovery.

Empirical calibration: the DT of a unit cube's 8 corners is 6 tetrahedra with
total volume exactly 1.0; only 4 of the 12 input triangles survive as DT faces
(the DT chooses its own square-face diagonals), confirming that *boundary
conformance* (surface respected) is achievable without *boundary preservation*
(exact input diagonals) — matching TetGen's `-p` vs `-Y` distinction.

## Pipeline

`cmg::cdt::tetrahedralize_plc(PLC, opts)`:

1. **Facet triangulation** — fan-triangulate each facet polygon into triangles
   (`triangles`), used both for the interior test and the boundary output.
   (Convex/triangle facets only this increment; general polygons deferred.)
2. **Delaunay** — `delaunay(PLC.points, opts)` (Phase 1 kernel).
3. **Interior carve** — for each tetrahedron, take its centroid and test
   point-in-domain by **ray casting**: shoot a fixed generic-direction ray and
   count Möller–Trumbore intersections with the boundary triangles; an odd count
   means inside. Keep inside tets. Facet-bounded voids yield even counts → removed.
4. **Boundary faces** — build face→kept-tet adjacency; a face bordering exactly one
   kept tet is a boundary face, emitted with marker 1. Interior faces are not
   emitted.
5. Return the carved `Mesh` (points = PLC points; only referenced-by-kept-tets
   vertices are used, unreferenced ones retained in `points`).

## Robustness notes

- The DT decisions use the exact predicates (Phase 1). The ray-cast test is a
  geometric heuristic on `double`; to avoid axis-aligned degeneracies the ray uses
  a fixed generic direction and a small epsilon, and the centroid (strictly
  interior to its tet) avoids on-boundary ambiguity for well-formed inputs.
- Volume conservation on convex domains is the primary correctness check: the sum
  of kept-tet volumes equals the domain volume within tolerance.

## Why not attempt Steiner recovery here

Inserting Steiner points to force missing boundary faces runs straight into
coplanar-facet degeneracies (a facet's interior Steiner point does not force its
sub-triangles to appear in the 3D DT) and non-termination on sharp dihedral angles.
Doing it *robustly* is the substance of the deferred increment; a fragile attempt
would produce wrong or non-terminating results. This increment therefore commits to
the class it can serve correctly and reports the rest as deferred, rather than
shipping an unreliable recovery.

## Testing

- **Single tetrahedron PLC** (4 triangle facets) → 1 tet.
- **Cube PLC** (12 triangles) → 6 tets, boundary faces on the cube surface, total
  volume 1.0 (convex exactness).
- **Carving removes tets**: a domain whose convex hull exceeds the domain (a
  facet-bounded concavity/void) loses the exterior tets; kept volume < hull volume
  and equals the domain volume.
- **Dogfood Phase 2**: load a cube `.smesh`/`.off` via `cmg::io`, tetrahedralize,
  check the result.
- **TetGen oracle**: for the convex cube, kept-tet volume matches TetGen's meshed
  volume (both = 1.0).
