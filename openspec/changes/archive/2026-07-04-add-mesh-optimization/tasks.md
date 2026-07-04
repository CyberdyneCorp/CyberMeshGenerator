# Tasks — Mesh optimization (Phase 7, increment 1)

## Engine
- [x] `cmg::optimize::laplacian_smooth(mesh, opts)` (include/cmg/optimize + src/optimize)
- [x] Interior/boundary vertex classification (boundary = in any mesh.faces)
- [x] Vertex neighbor + incident-tet adjacency from tet edges
- [x] Inversion-guarded relocation (exact-orient3d sign preserved; damped step)
- [x] `cmg::optimize::min_dihedral_angle(mesh)` quality measure
- [x] Wire into CMake + umbrella header

## Tests
- [x] Boundary fixed, no inversions, volume conserved (refined cube)
- [x] min_dihedral_angle not worsened by smoothing
- [x] Interior vertex moves toward neighbor centroid (inversion-safe case)
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Edge/face flips (2-3 / 3-2 topological transforms)
- [ ] Sliver-targeted / smart-Laplacian / ODT / CVT smoothing
- [ ] Boundary-vertex smoothing along facets/segments
- [ ] Second-order (-o2) nodes
