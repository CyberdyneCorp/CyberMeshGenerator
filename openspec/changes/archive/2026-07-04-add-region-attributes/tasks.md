# Tasks — Region attributes and holes (Phase 6, increment 1)

## Classification engine
- [x] `cmg::region::apply(Mesh&, const PLC&, bool label_regions)` module (include/cmg/region + src/region)
- [x] Face-adjacency build + connected components (union-find) over kept tets
- [x] Seed point location (exact-orient3d barycentric) → component
- [x] Hole-seed component removal
- [x] Region-seed attribute assignment into tet_markers; auto-label unseeded components under flag
- [x] Recompute boundary faces after removal
- [x] `MeshOptions::label_regions` field (-AA analogue)
- [x] Call from the PLC pipeline (after carve) when regions/holes present or label_regions set; wire CMake

## Tests
- [x] Single region: cube seed attribute 5 → all tets marked 5
- [x] Two separated solids: attributes 1 and 2 assigned per solid
- [x] Hole seed removes its solid, keeps the other
- [x] Auto-label: two distinct nonzero labels, no seeds
- [x] No regions/holes ⇒ markers 0, mesh unchanged
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Regions separated only by an internal facet absent from the mesh (needs CDT recovery)
- [ ] Per-region maximum-volume constraints (region membership during refinement)
- [ ] Exterior -1 marking under -c (retain convex hull)
