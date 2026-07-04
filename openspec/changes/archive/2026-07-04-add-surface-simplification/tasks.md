# Tasks — Surface simplification

- [x] Core: `cmg::simplify::simplify(PLC, SimplifyOptions)` — grid vertex clustering, deterministic
- [x] CMake: add `src/simplify/simplify.cpp` to the library
- [x] Core test: simplifying a dense sphere/cube reduces triangle count and preserves enclosed volume within tolerance; determinism for a fixed grid
- [x] C ABI: `cmg_plc_simplify` returning a new PLC handle
- [x] Python: `cybermesh.simplify(plc, grid=...)` → `PLC`
- [x] Swift: `CyberMesh.simplify(_ plc:, grid:)` → `PLC` (source-only; no Swift toolchain here)
- [x] Python test: simplify a loaded PLC, triangle count drops, still meshable
- [x] Migrate Bunny/Eiffel examples to `cm.simplify` (drop local `cluster_decimate`)
- [x] `openspec validate --all --strict` green

## Deferred
- [ ] Quality-preserving decimation (quadric error / edge collapse); watertightness guarantees
