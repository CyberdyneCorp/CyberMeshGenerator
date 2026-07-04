# Bootstrap CyberMeshGenerator foundation

## Why

CyberMeshGenerator is a clean-room **Modern C++20 port of TetGen** — the
Delaunay-based quality tetrahedral mesh generator — and the meshing sibling of
NumPP (NumPy) and SciPP (SciPy). TetGen is a large, mature body of computational
geometry (~36k lines of single-file C++ plus Shewchuk's exact predicates),
spanning Delaunay/constrained-Delaunay tetrahedralization, quality refinement,
optimization, coarsening, reconstruction, Voronoi duals, a dozen file formats, and
a switch-driven CLI. Porting it all at once would be unreviewable, so we commit to
a **phased roadmap** and detail only the foundation now.

The foundation locks in the cross-cutting decisions every later capability depends
on:

1. **TetGen is the oracle.** Like NumPP↔NumPy and SciPP↔SciPy, every result is
   validated against real TetGen 1.6.0 (`/home/leonardo/work/TetGen`), whose
   OpenSpec baseline is the behavioral contract we port against. Exact predicates
   must return identical signs; tetrahedralizations must be combinatorially
   equivalent under documented tie-breaking.

2. **Runs everywhere: pure CPU, GPU desktop, and mobile.** One source tree, three
   build profiles — mobile (CPU-only, zero extra deps, builds on iOS/Android),
   desktop, workstation (+GPU) — selected at runtime. The portable CPU kernel is
   always present and is the only thing required to build.

3. **Tiered acceleration with CUDA / OpenCL / Metal.** The parallelizable phases
   TetGen runs serially gain optional GPU backends, routed through **NumPP's
   existing weak-linked `CapabilityRegistry` + `GpuVTable`**, so the library still
   compiles and runs CPU-only on iOS/Android and the GPU result always equals the
   CPU path within documented tolerance.

4. **Modern, type-safe C++20 API — plus Python and Swift bindings.** Not char
   switches: a typed `MeshOptions`/`PLC`/`Mesh` API with `std::span`, strong enums,
   and `std::expected`. First-class Python (NumPy-interoperable) and Swift
   (iOS/macOS) bindings are foundational deliverables, each a thin layer over the
   same core.

5. **Exact predicates are a correctness invariant.** Shewchuk's adaptive
   predicates are ported verbatim in an isolated `-O0` translation unit so the
   optimizer cannot break their robustness guarantees.

This change establishes Phase 0 (skeleton, data model, build/packaging, NumPP/SciPP
integration, backend-acceleration architecture, robust predicates, language-binding
architecture, TetGen oracle harness) and records the full TetGen parity backlog.
Everything else (Delaunay, CDT, quality, optimization, Voronoi, file formats, CLI)
follows as separate OpenSpec changes against this baseline.

## What changes

This change introduces the following capabilities (spec deltas):

- **mesh-core-foundation** — the project skeleton and the Modern C++20 data model
  (`Point3`, `Mesh`, `PLC`, `MeshOptions`, `TetHandle`), the NumPP/SciPP
  integration contract (optional; core builds standalone), CMake ≥ 3.25 / C++20
  layout, feature flags, Conan + vcpkg packaging, the iOS/Android CPU-only build
  contract, and the `cmg::error` / `std::expected` result model.

- **robust-geometric-predicates** — Shewchuk's exact/adaptive `orient3d`,
  `insphere` (and 2D counterparts) ported verbatim, compiled in an isolated `-O0`
  translation unit, initialized once at startup, with a batched
  device-evaluable interface for the accelerated hot path. Correctness-critical.

- **backend-acceleration** — how CyberMeshGenerator's parallelizable kernels
  (batched predicate evaluation, BRIO-Hilbert spatial sort, point location,
  independent-set point insertion, quality histograms) reuse NumPP's
  `CapabilityRegistry`, `last_backend()`, `NUMPP_GPU_TARGET` override and device
  buffer pool for **CUDA, OpenCL and Metal**, always with a portable CPU fallback
  and documented cross-backend result equivalence.

- **language-bindings** — the Python and Swift binding architecture: a NumPy-
  interoperable Python module (arrays via NumPP `.npy`/DLPack, `PLC`/`Mesh`/options
  as Python objects) and an idiomatic Swift package for iOS/macOS, each a thin,
  independently-versioned layer over a stable C ABI shim over the C++ core.

- **tetgen-oracle** — the TetGen validation harness: tests express expected
  results by running real TetGen 1.6.0 and asserting topological/numeric
  equivalence, with a frozen/checked mode so CI can run without a TetGen build,
  plus the tie-breaking and tolerance conventions for degenerate inputs.

- **tetgen-parity** — the prioritized, tiered backlog of TetGen's capabilities with
  a suggested implementation order, so gap-closing work flows through OpenSpec as
  discrete changes.

## Phased roadmap (scope guardrail)

Only Phase 0 (foundation) is specified in detail here. Later phases are the agreed
roadmap and each arrives as its own OpenSpec change. **Do not implement Phase 1+
capabilities in this change.**

| Phase | Title | This change? |
|------:|-------|:---:|
| 0 | Foundation: data model, build/packaging, NumPP/SciPP integration, robust predicates, backend acceleration, bindings architecture, TetGen oracle | ✅ |
| 1 | `robust-geometric-predicates` full validation + `delaunay-tetrahedralization` (Bowyer-Watson DT, weighted/regular DT, BRIO-Hilbert sort) — the kernel everything builds on | ⬜ later |
| 2 | `file-formats` (`.node`/`.poly`/`.smesh`/`.ele`/`.face`/`.edge`, STL/OFF/PLY/VTK/Medit) — needed to feed/verify every other capability | ⬜ later |
| 3 | `constrained-tetrahedralization` (CDT of a PLC, segment/facet recovery, boundary conforming) | ⬜ later |
| 4 | `quality-mesh-generation` (`-q` radius-edge / dihedral bounds, `-a` volume constraints, Steiner budget) | ⬜ later |
| 5 | `adaptive-mesh-sizing` (background-mesh sizing function, per-facet/segment constraints) | ⬜ later |
| 6 | `region-attributes` (region marking, material attributes, hole carving) | ⬜ later |
| 7 | `mesh-optimization` (smoothing, flips, quality improvement) + `mesh-coarsening` (vertex removal) | ⬜ later |
| 8 | `mesh-reconstruction` (re-mesh / refine an existing mesh, `.vol` constraints) | ⬜ later |
| 9 | `voronoi-diagram` (Voronoi / power-diagram dual output) | ⬜ later |
| 10 | `command-line-interface` (optional TetGen-compatible `tetgen`-style CLI + switch parser) | ⬜ later |
| 11 | GPU device kernels for the accelerable phases (CUDA/OpenCL/Metal), `language-bindings` completion, hardening, docs, v1.0 packaging | ⬜ later |

## Reuse vs rewrite

Per project policy, prefer adapting proven code over reinventing — after testing:

- **Port from TetGen** (`/home/leonardo/work/TetGen`): the algorithms themselves —
  incremental Bowyer-Watson insertion, BRIO-Hilbert sort, constrained-Delaunay
  recovery, Delaunay refinement, flip/smoothing optimization — re-expressed in
  idiomatic C++20; the documented behavior, edge cases, and tie-breaking captured
  in its OpenSpec baseline.
- **Port verbatim**: `predicates.cxx` (Shewchuk adaptive predicates). These are
  battle-tested and correctness-critical; they are transcribed with minimal change
  and isolated at `-O0`, not rewritten.
- **Reuse from NumPP** (`/home/leonardo/work/NumPP`): the backend-dispatch
  substrate (`CapabilityRegistry`, weak-linked `GpuVTable`, `last_backend()`,
  `NUMPP_GPU_TARGET`, device buffer pool), `ndarray` for point/tet arrays and
  binding interop, and the build/packaging skeleton (CMake options, Conan, vcpkg,
  `config.hpp`).
- **Reuse from SciPP** (`/home/leonardo/work/SciPP`): `scipp::spatial` primitives
  (KD-tree, pairwise distances) where useful for point location and sizing, when
  the optional integration is enabled.
- **Rewrite**: TetGen's global mutable state, C-style memory pools, `char*`
  switch strings, and file-extension-driven control flow — replaced with RAII
  containers, a typed options struct, and `std::expected` error handling.

## Non-goals

- **No** Phase 1+ capability implementation here — this change is foundation +
  roadmap only. No Delaunay kernel, no CDT, no refinement in this PR.
- **Not** a line-for-line transliteration of TetGen's single-file design or its
  global mutable state; the algorithms are ported, the architecture is modernized.
- **Not** re-implementing arrays/dtypes/GPU device management — the device
  substrate comes from NumPP; this change specifies the *integration*, not a new
  vtable.
- **No** source/ABI compatibility with TetGen's `tetgenio`/`tetgenbehavior` C++
  classes; a TetGen-compatible CLI and file I/O are a later, optional capability.
- **No** rewrite of Shewchuk's predicates — they are ported verbatim.
- **No** license decision baked in — AGPLv3 vs. relicensing is an open question
  for the maintainer (see design), tracked as a blocking pre-distribution item.
