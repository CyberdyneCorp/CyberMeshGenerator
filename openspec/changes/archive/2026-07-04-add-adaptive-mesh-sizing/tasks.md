# Tasks — Adaptive mesh sizing (Phase 5, increment 1)

## Sizing engine
- [x] `MeshOptions::sizing` field (`std::function<double(const Point3&)>`, edge-length target)
- [x] Generalize `quality::refine` bad-tet test to a per-tet volume target from sizing (h³/6√2), min with global `max_volume`
- [x] Treat `sizing` as refinement-requested in `delaunay()` / `tetrahedralize()`
- [x] `cmg::sizing::from_background(mesh, node_sizes, scale)` — point-locate + barycentric interpolation, nearest-node outside
- [x] `include/cmg/sizing` + `src/sizing`; wire into CMake

## Tests
- [x] Graded analytic sizing: fine half has smaller/more tets than coarse half
- [x] Size 0 leaves a region coarse
- [x] Background-mesh sizing interpolates (exact at nodes) and refines the small-size region
- [x] Metric scaling halves sizes
- [x] Sizing + max_volume: tighter target wins
- [x] No sizing + no volume ⇒ unrefined
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] `.mtr` / `.b.*` file parsing (file-formats follow-up)
- [ ] Anisotropic / tensor metrics
- [ ] Size-field gradient limiting / smoothness control
