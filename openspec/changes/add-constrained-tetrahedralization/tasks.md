# Tasks — Constrained/boundary-conforming tetrahedralization (Phase 3, increment 1)

## Pipeline
- [x] `cmg::cdt::tetrahedralize_plc(PLC, MeshOptions)` module (include/cmg/constrained + src/constrained)
- [x] Facet fan-triangulation into constraint triangles
- [x] DT of PLC vertices (reuse Phase 1 `delaunay`)
- [x] Ray-cast interior classification (Möller–Trumbore, generic direction, centroid test)
- [x] Boundary-face extraction (faces bordering exactly one kept tet), marker 1
- [x] Route `tetrahedralize(PLC)` faceted case to the pipeline (remove NotImplemented stub)

## Tests
- [x] Single-tetrahedron PLC → 1 tet
- [x] Cube PLC → volume 1.0, boundary faces on the cube surface, valid orientation
- [x] Carving removes exterior tets (hull volume > domain volume case)
- [x] Dogfood Phase 2: load a cube `.smesh`/`.off` via `cmg::io`, then tetrahedralize
- [x] Degenerate (coplanar) PLC returns InvalidInput
- [x] TetGen oracle: convex-cube kept volume matches TetGen's meshed volume
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred (tracked for later increments)
- [ ] Exact facet preservation (`-Y`) + true constrained-Delaunay boundary recovery
- [ ] Exact non-convex boundary conformance (facet/segment recovery, Steiner points)
- [ ] Seed-only holes/regions + region-attribute assignment (Phase 6)
- [ ] General (non-convex) facet polygon triangulation
