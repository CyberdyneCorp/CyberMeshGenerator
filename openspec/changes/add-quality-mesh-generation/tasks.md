# Tasks — Quality mesh generation (Phase 4, increment 1: volume refinement)

## Refinement engine
- [x] `cmg::quality::refine(points, plc?, opts)` module (include/cmg/quality + src/quality)
- [x] Circumcenter + tet-volume computation (guarded against slivers)
- [x] Over-large-tet detection against `max_volume`
- [x] Point-in-domain test (inside some current tet) for admissible circumcenters
- [x] Centroid fallback for boundary tets whose circumcenter is outside the domain
- [x] Near-duplicate guard so refinement cannot stall on collapsing insertions
- [x] Refinement loop with Steiner budget + MAX_ROUNDS + no-progress stop; re-mesh each round
- [x] Wire `delaunay()` / `tetrahedralize()` to refine when `max_volume` set (bypass 4-pt fast path)

## Tests & oracle
- [x] Volume bound: cube `max_volume=0.05` → all tets ≤ 0.05, volume 1.0, count grew
- [x] Steiner budget caps insertions
- [x] No options ⇒ unrefined mesh (no Steiner points)
- [x] `quality`-only (no volume) ⇒ accepted, returns unrefined mesh (no divergence)
- [x] Refinement stays inside the domain (volume conserved)
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred (later increments)
- [ ] Radius-edge / shape refinement (needs sliver exudation + encroachment; diverges naively)
- [ ] Minimum dihedral angle / sliver removal
- [ ] Boundary encroachment protection; provable non-convex termination
- [ ] Per-region / per-facet / per-segment size constraints
- [ ] Background-mesh sizing function (Phase 5)
