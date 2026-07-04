# Tasks — Mesh reconstruction (Phase 9)
- [x] `cmg::reconstruct::reconstruct(mesh, opts)` (include/cmg/reconstruct + src/reconstruct): re-DT the vertex set + apply refinement/sizing
- [x] Wire into CMake
- [x] Tests: reconstruct+refine to a finer max_volume -> all tets meet bound, all input vertices retained; default reproduces DT
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Exact in-place connectivity preservation; `.vol`-driven per-tet refinement
