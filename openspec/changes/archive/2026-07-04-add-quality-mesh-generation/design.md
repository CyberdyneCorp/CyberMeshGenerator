# Design — quality mesh generation (Delaunay refinement)

## Approach

Classic Delaunay refinement: repeatedly find an over-large tetrahedron and insert a
new vertex to split it. Reuses the Phase 1–3 kernels to re-mesh after each batch of
insertions, so refinement inherits their exact-predicate robustness and the domain
carving.

**This increment refines on VOLUME only.** Shape (radius-edge) refinement is
deferred: enforcing a radius-edge bound by inserting circumcenters of skinny
tetrahedra diverges without sliver-exudation + encroachment handling (a flat sliver
has a huge ratio; splitting it makes more slivers — measured to blow up to ~13k
tetrahedra with the ratio still rising). Volume refinement has no such pathology:
subdivision strictly reduces tetrahedron volume toward the bound, and slivers have
small volume so they are never targeted — it terminates and conserves volume.

```
refine(points, plc?, opts):
  bound_re  = opts.quality.radius_edge   (default 2.0, if quality set)
  bound_vol = opts.max_volume            (if set)
  budget    = opts.steiner_budget        (default cap otherwise)
  loop up to MAX_ROUNDS:
    mesh = remesh(points)                # delaunay(points) or carved PLC mesh
    bad  = tets with volume > bound_vol
    if bad empty: return mesh
    added = 0
    for t in bad (largest first):
      site = circumcenter(t) if inside domain else centroid(t)   # centroid always inside
      if not near an existing point:
        points.push_back(site); ++added
        if points.size()-n0 >= budget: break
    if added == 0: return mesh          # converged / budget
  return mesh
```

- **remesh** strips `quality`/`max_volume` from the options (to avoid recursion):
  a point set → `delaunay(points)`; a PLC → `tetrahedralize_plc(plc_with(points))`
  (DT + ray-cast carve).
- **inside_domain(mesh, p)** = `p` lies inside some tetrahedron of the current
  (already-carved) mesh, tested with exact `orient3d` barycentric signs. This one
  rule serves both cases: for a point set the union of tets is the convex hull; for
  a PLC it is the carved domain. Circumcenters outside the domain are skipped.
- **near an existing point**: skip a circumcenter within a small fraction of the
  domain extent of an existing vertex, so refinement cannot stall on
  near-duplicate insertions (the DT would collapse exact duplicates anyway).

## Circumcenter and radius-edge

For a tetrahedron `a,b,c,d` with `B=b−a, C=c−a, D=d−a`:

```
cc = a + ( |B|²(C×D) + |C|²(D×B) + |D|²(B×C) ) / ( 2 · B·(C×D) )
```

Circumradius `R = |cc − a|`; radius-edge ratio `= R / (shortest of the 6 edges)`.
A degenerate (near-zero-volume) tet has `B·(C×D) ≈ 0`; such slivers are left in
place this increment (sliver removal is deferred), guarded against division blow-up.

## Termination

Radius-edge refinement with a bound ≥ 2 provably terminates (Shewchuk); volume
refinement terminates because each insertion strictly reduces the max volume toward
the bound. The Steiner budget and `MAX_ROUNDS` cap are hard backstops, and the
`added == 0` check stops when no admissible circumcenter remains (e.g. all bad tets
are boundary slivers whose circumcenter is outside the domain). The result is always
a valid mesh; if the budget is hit before the bound is met, the best mesh so far is
returned (matching TetGen's `-S` behavior).

## Wiring

`delaunay()` / `tetrahedralize()` call `quality::refine` when `quality` or
`max_volume` is set; otherwise they behave exactly as before. The 4-point fast path
is bypassed when refinement is requested.

## Testing

- **Volume bound**: cube with `max_volume = 0.05` → every tet ≤ 0.05, total volume
  still 1.0, tet count grew.
- **Radius-edge bound**: a mesh refined to `radius_edge = 1.5` → every tet meets the
  ratio (within the budget); tighter bound ⇒ more tets.
- **Budget cap**: `steiner_budget = 20` → at most 20 Steiner points added.
- **No-op**: without quality/volume options the mesh is unchanged from Phase 1–3.
- **Domain preserved**: refinement conserves the domain volume and keeps the mesh
  inside the domain.
