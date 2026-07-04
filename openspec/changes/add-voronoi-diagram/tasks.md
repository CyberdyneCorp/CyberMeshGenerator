# Tasks — Voronoi diagram (Phase 9)

## Construction
- [x] Shared circumcenter geometry helper (used by refinement + Voronoi)
- [x] `cmg::voronoi::VoronoiDiagram` + `build(const Mesh&)` (include/cmg/voronoi + src/voronoi)
- [x] Vertices: circumcenter per tet, degenerate-guarded (finite only)
- [x] Edges: face-adjacency; finite edge for interior faces, outward ray for hull faces
- [x] Cells: per input vertex, incident tetrahedra
- [x] Wire into CMake + umbrella header

## Tests
- [x] One vertex per tet, each equidistant from its tet's 4 vertices
- [x] Interior faces → finite edges; hull faces → outward rays (count + direction)
- [x] Cells non-empty per used vertex; cover all tets
- [x] No non-finite vertices on a near-degenerate input
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Voronoi facets (dual to Delaunay edges; ordered rings)
- [ ] Power / weighted diagram (orthocenters, -vw)
- [ ] Voronoi file output (.v.node/.v.edge/.v.face/.v.cell)
- [ ] Clipping rays to a bounded box
