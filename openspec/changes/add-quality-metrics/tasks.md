# Tasks — Mesh quality metrics and reporting
- [x] `cmg::quality::report(mesh, worst_count)` (include/cmg/quality/metrics.hpp + src/quality/metrics.cpp): radius-edge (shared circumcenter), dihedral (reuse the 6-angle computation), volume; min/max/mean, histogram, worst-N by radius-edge
- [x] Degenerate-tet handling (no NaN/inf)
- [x] Wire into CMake + umbrella header
- [x] Tests: histogram sums to 6*num_tets, total_volume = sum, ordering invariants; regular-tet ~70.5deg / ~0.61; worst_tets descending
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Radius-edge / dihedral shape refinement (diverges naively; needs queue-based Ruppert + encroachment + sliver handling)
- [ ] Anisotropic quality measures
