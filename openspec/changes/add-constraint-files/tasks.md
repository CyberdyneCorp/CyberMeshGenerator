# Tasks — Constraint/sizing file I/O (.vol, .mtr)

- [x] `read_vol` / `write_vol` (src/io/constraints.cpp) using detail:: helpers
- [x] `read_mtr` / `write_mtr` (same TU)
- [x] Declarations in formats.hpp; wire into CMake
- [x] Tests: .vol round-trip + count-mismatch error; .mtr round-trip
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] `.var` per-facet-area / per-segment-length constraints
- [ ] Anisotropic (tensor) `.mtr`
- [ ] Pipeline wiring (load .mtr -> sizing function; .vol -> reconstruction)
