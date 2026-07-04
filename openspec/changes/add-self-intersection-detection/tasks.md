# Tasks — PLC self-intersection detection (-d)
- [x] `cmg::detect::self_intersections(plc)` (include/cmg/detect + src/detect/self_intersection.cpp)
- [x] Fan-triangulate facet polygons; exact-predicate triangle-triangle intersection (orient3d-based)
- [x] Exclude adjacent triangles (sharing a vertex or edge) from being flagged
- [x] Wire into CMake + umbrella header
- [x] Tests: two crossing triangles detected; valid cube/tet PLC -> empty; edge-sharing not flagged
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Facet-interior recovery / true CDT facet conformance (needs 2-D Delaunay per facet + conforming refinement)
- [ ] Intersection repair; coplanar-overlap classification
