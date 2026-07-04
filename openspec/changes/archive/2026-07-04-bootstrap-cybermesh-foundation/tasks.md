# Tasks — Bootstrap CyberMeshGenerator foundation

## Phase 0 — Foundation (this change)

### Project skeleton & build
- [x] CMake ≥ 3.25 / C++20 project; `include/cmg/` + `src/<capability>/` layout with reserved capability dirs
- [x] Generated `config.hpp` with backend flags `CMG_WITH_{CUDA,OPENCL,METAL}`, integration flags `CMG_WITH_{NUMPP,SCIPP}`, binding flags `CMG_WITH_{PYTHON,SWIFT}`, precision flag `CMG_SINGLE` (all default OFF)
- [x] Umbrella `cmg/cmg.hpp`, `fwd.hpp`, `version.hpp.in`
- [x] NumPP/SciPP wired as **optional pinned Conan/vcpkg release dependencies** resolved via `find_package`; core builds with both OFF
- [x] Configure-time check: requested GPU flag implies `CMG_WITH_NUMPP` + matching `NUMPP_WITH_<GPU>`; fail fast otherwise
- [x] CPU-only, no-integration build verified green (default + single-precision `just mobile`); iOS/Android cross-compile contract documented in README
- [x] `justfile` entrypoints (`build`/`test`/`ctest`/`asan`/`oracle`/`spec`/`ci`/`mobile`/`clean`)

### Data model & error handling
- [x] `Point3`, `Mesh`, `PLC` (facets/holes/regions), `MeshOptions`, `Quality`, handles — value types, RAII, move-safe
- [x] `tetrahedralize(PLC, MeshOptions) -> cmg::expected<Mesh, MeshError>` and `delaunay(span<Point3>, MeshOptions)` signatures (`cmg::expected` aliases `std::expected` on C++23, bundled fallback on C++20)
- [x] `MeshOptions::from_switches` TetGen-compatible parser returning `cmg::expected<…, ParseError>`
- [x] `cmg::error` exception base + `MeshError`/`ParseError` result types; no `exit()`/`longjmp`; NumPP-compatible catch base under `CMG_WITH_NUMPP`
- [x] No global mutable meshing state (per-call locals); thread-safe one-time predicate init; concurrency-safe `tetrahedralize`

### Robust predicates
- [x] Port `predicates.cxx` verbatim to `src/predicates/predicates.cpp` (namespacing + `REAL` typedef only)
- [x] Thread-safe, idempotent one-time `exactinit` equivalent (`robust::ensure_initialized`, `std::call_once`)
- [x] Compile the predicate TU at `-O0` (`set_source_files_properties`) while the rest builds `-O3` — verified via compile_commands.json
- [x] Coplanarity tolerance (default `1e-8`) and diagnostic predicate-mode option (`-X`/`-X1` analogue) in `MeshOptions`
- [x] Batched `orient3d_batch` interface (exact CPU path; device fast-filter slot) — tested equal to per-item evaluation

### Backend acceleration
- [x] Dispatch shim over NumPP's `CapabilityRegistry` / `last_backend()` / `NUMPP_GPU_TARGET` / device pool (guarded, CPU-only path active by default)
- [x] Accelerable-op enum + threshold slots (batched predicates, Hilbert-key sort, point location, quality scan) — interface only, no meshing impl
- [x] Size-threshold-gated dispatch policy + CPU-fallback guarantee (`should_offload`, always CPU on no-GPU build)
- [x] Deterministic-topology-across-backends contract documented; `last_backend()` test pattern established
- [~] Optional multithreaded CPU path (`CMG_WITH_THREADS` flag + Threads link); actual threaded kernels land with Phase 1

### Language bindings (architecture)
- [x] Stable C ABI shim (`bindings/c/`): opaque handles, `extern "C"` entry points, error-code + message convention — pure-C smoke test green
- [x] Python module scaffold (`bindings/python/`, `pyproject.toml`/scikit-build-core; ctypes NumPy-interop reference + nanobind wheel path)
- [x] Swift package scaffold (`bindings/swift/`, SwiftPM) over the C ABI; iOS/macOS targets; Metal-on-device + CPU fallback contract
- [x] Verify core builds with both binding flags OFF; C ABI builds independently via `CMG_BUILD_C_ABI`

### TetGen oracle harness
- [~] Zero-dependency runner shipped (`tests/harness.hpp`); Catch2 + live TetGen invocation layered on in Phase 1 (documented in `tests/oracle/README.md`)
- [x] Equivalence criteria documented (exact predicate signs, general-position tuple-set topology, counts/quality histograms, Delaunay invariants)
- [x] Degenerate-input tie-breaking + non-unique-case documentation convention (`tests/oracle/README.md`)
- [~] Frozen/`tests/oracle/golden/` mode contract + `CMG_ORACLE_FROZEN` switch defined; golden corpus populated with Phase 1
- [x] `openspec validate --all --strict` green; GitHub Actions CI gate added (spec + cpu matrix + asan + mobile)

### Pre-distribution blockers (flag, do not resolve here)
- [x] License decision documented as an OPEN maintainer-owned blocker in `NOTICE.md` (AGPLv3 vs. clean-room relicensing) — decision itself deferred to maintainer
- [x] Predicate provenance recorded (Shewchuk public-domain) in `NOTICE.md`; exact-terms confirmation flagged for maintainer

## Roadmap — each item graduates into its own OpenSpec change

> Not implemented in this change. Ported against the TetGen oracle and its OpenSpec
> baseline; GPU-accelerable phases reuse the backend-acceleration substrate.

- [ ] **Phase 1** — `delaunay-tetrahedralization` (Bowyer-Watson DT, weighted/regular DT, BRIO-Hilbert sort, convex hull)
- [ ] **Phase 2** — `file-formats` (`.node`/`.poly`/`.smesh`/`.ele`/`.face`/`.edge`/`.vol`/`.var`, STL/OFF/PLY/VTK/Medit)
- [ ] **Phase 3** — `constrained-tetrahedralization` (CDT of a PLC, segment/facet recovery, boundary conforming)
- [ ] **Phase 4** — `quality-mesh-generation` (`-q` radius-edge/dihedral, `-a` volume, per-facet/segment, Steiner budget)
- [ ] **Phase 5** — `adaptive-mesh-sizing` (background-mesh sizing function, metric-driven refinement)
- [ ] **Phase 6** — `region-attributes` (region marking, material attributes, hole carving, convex retention)
- [ ] **Phase 7** — `mesh-optimization` (smoothing, flips, quality improvement, second-order nodes)
- [ ] **Phase 7** — `mesh-coarsening` (vertex removal / decimation)
- [ ] **Phase 8** — `mesh-reconstruction` (re-mesh/refine existing mesh, `.vol`/`.var` refinement)
- [ ] **Phase 9** — `voronoi-diagram` (Voronoi / power-diagram dual)
- [ ] **Phase 10** — `command-line-interface` (optional TetGen-compatible executable + switch parser)
- [ ] **Phase 11** — GPU device kernels (CUDA/OpenCL/Metal) for accelerable phases; `language-bindings` surface completion; hardening; docs; v1.0 packaging
