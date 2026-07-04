# Tasks — Mesh coarsening (Phase 8)
- [x] `cmg::coarsen::coarsen(mesh, opts)` (include/cmg/coarsen + src/coarsen): classify interior vs boundary, drop a seeded fraction of interior, re-DT the rest
- [x] Wire into CMake
- [x] Tests: fewer tets + boundary retained; determinism for a fixed seed
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Quality/error-driven vertex selection; boundary decimation; exact element-count target
