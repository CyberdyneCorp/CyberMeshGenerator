# Tasks — Voronoi output and power diagram

- [x] `cmg::io::write_voronoi` (src/io/voronoi_io.cpp): .v.node, .v.edge (rays v2=-1 + dir), .v.cell
- [x] `cmg::voronoi::build_power(mesh, weights)` (src/voronoi/power.cpp): orthocenter vertices, same edge/cell duality
- [x] Wire into CMake
- [x] Tests: write_voronoi emits vertices + finite/ray edges; power diagram counts match Voronoi structure, orthocenter equidistance in the weighted metric
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] `.v.face` full ordered facet rings
- [ ] Reading `.v.*` back
- [ ] Ray clipping to a bounded box
