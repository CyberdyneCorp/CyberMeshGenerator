# Tasks — Accelerate the PLC carve

- [x] Build a per-ray-direction 2-D bucket grid over boundary triangles (project onto the plane ⟂ the ray)
- [x] Route `inside_domain` through the grid: a query tests only triangles in its cell
- [x] Confirm classification is identical to brute force (superset-of-hits guarantee) — same meshes
- [x] Regression test: a full-resolution (non-decimated) closed surface meshes; volume matches the decimated result within tolerance
- [x] Regression test: existing cube/tetrahedron carve results unchanged
- [x] Examples: decimation is now optional — Bunny/Eiffel can raise or drop the grid
- [x] `openspec validate --all --strict` green; all C++ tests green (default / -Werror / ASan / single precision)

## Deferred
- [ ] Exact concave boundary recovery; general BVH
