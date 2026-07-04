# CyberMeshGenerator — Project Context

## What this is

CyberMeshGenerator (namespace `cmg`) is a clean-room **Modern C++20 port of
[TetGen](https://codeberg.org/TetGen/TetGen)** — a Delaunay-based quality
tetrahedral mesh generator and 3D Delaunay/Voronoi engine. It re-implements
TetGen's proven algorithms behind a modern, type-safe C++20 API that runs
**everywhere**: pure CPU, GPU desktops/workstations, and mobile (iOS, Android),
with optional **CUDA / OpenCL / Metal** acceleration and first-class **Python**
and **Swift** bindings.

It is the meshing sibling of the CyberdyneCorp scientific stack:

- **[NumPP](https://github.com/CyberdyneCorp/NumPP)** — C++20 NumPy port: N-D
  `ndarray`, dtypes, ufuncs, linalg, fft, random, with a tiered CPU / BLAS /
  **CUDA / OpenCL / Metal** acceleration substrate (`CapabilityRegistry`,
  weak-linked `GpuVTable`, `last_backend()`, `NUMPP_GPU_TARGET`). Available at
  `/home/leonardo/work/NumPP`.
- **[SciPP](https://github.com/CyberdyneCorp/SciPP)** — C++20 SciPy port built on
  NumPP; its `scipp::spatial` (KD-tree, distances, ConvexHull/Delaunay) is
  directly relevant to mesh point location and sizing. Available at
  `/home/leonardo/work/SciPP`.

CyberMeshGenerator **reuses NumPP's device-dispatch substrate** instead of
building its own, and **optionally consumes SciPP** spatial primitives. Both are
optional: the core mesher builds CPU-only with zero extra dependencies.

## Relationship to the reference TetGen (the oracle)

The upstream TetGen 1.6.0 checkout at `/home/leonardo/work/TetGen` is the
**oracle and reference implementation**. Its OpenSpec baseline
(`/home/leonardo/work/TetGen/openspec/specs/`) documents the observed behavior of
each capability and is the behavioral contract CyberMeshGenerator ports against.

- CyberMeshGenerator re-implements the *algorithms* — incremental Bowyer-Watson
  Delaunay insertion, BRIO-Hilbert spatial sorting, constrained Delaunay,
  Delaunay refinement, mesh optimization — in idiomatic C++20 (RAII, `std::span`,
  strong types, `std::expected`), **not** the C-style single-file design.
- **Numerical results are validated against TetGen**: for the same input and
  equivalent options, the port's tetrahedralization matches TetGen's (identical
  topology in general position; combinatorially equivalent under documented
  tie-breaking), and exact predicates return identical signs.
- Spec requirements cite the ported TetGen source as a breadcrumb, e.g.
  `(oracle: tetgen.cxx:3266)` or `(oracle: tetgen.h)` / `(oracle: manual §4.2.3)`.

## Capability map

Each capability corresponds to a TetGen capability (mirrored from TetGen's
OpenSpec baseline), ported into the `cmg` namespace:

| Capability | TetGen source of truth | Scope |
|------------|------------------------|-------|
| mesh-core-foundation | tetgen.h data model | Modern C++20 skeleton, `Mesh`/`PLC`/typed options, NumPP/SciPP integration, build profiles, error model |
| robust-geometric-predicates | predicates.cxx | Exact/adaptive `orient3d`/`insphere`; correctness-critical |
| backend-acceleration | — (new) | Tiered CPU / CUDA / OpenCL / Metal for parallelizable kernels, reusing NumPP dispatch |
| language-bindings | — (new) | Python (NumPy-interop) + Swift bindings |
| tetgen-oracle | — (new) | Validation harness vs TetGen 1.6.0 |
| tetgen-parity | whole project | Prioritized backlog of TetGen capabilities |
| delaunay-tetrahedralization | delaunay-tetrahedralization | DT / weighted DT / BRIO-Hilbert sort |
| constrained-tetrahedralization | constrained-tetrahedralization | CDT of a PLC, boundary recovery |
| quality-mesh-generation | quality-mesh-generation | `-q`/`-a` refinement, Steiner points |
| adaptive-mesh-sizing | adaptive-mesh-sizing | Background mesh / sizing function |
| mesh-optimization | mesh-optimization | Smoothing, flips, quality improvement |
| mesh-coarsening | mesh-coarsening | Vertex removal / decimation |
| mesh-reconstruction | mesh-reconstruction | Re-mesh / refine an existing mesh |
| region-attributes | region-attributes | Region marking, material attributes |
| voronoi-diagram | voronoi-diagram | Voronoi / power-diagram dual |
| file-formats | file-formats | `.node`/`.poly`/`.smesh`/`.ele`/`.face`/STL/OFF/PLY/VTK/Medit I/O |
| command-line-interface | command-line-interface | Optional TetGen-compatible `tetgen`-style CLI |

## What CyberMeshGenerator does differently from TetGen

- **Modern C++20 API, not char-switch strings.** TetGen is driven by
  single-character switches (`-pq1.414a0.1`). The port exposes a typed, discoverable
  API (`MeshOptions` builder, strong enums, `std::expected` results); a
  TetGen-compatible switch parser is offered only as an optional compatibility layer.
- **One source tree, three build profiles** — mobile (CPU-only, zero extra deps;
  builds on iOS/Android), desktop (+BLAS/threads), workstation (+GPU) — selected
  at runtime. Acceleration is strictly additive; the portable CPU kernel is always
  present and is the only thing required to build.
- **CUDA / OpenCL / Metal acceleration** for the parallelizable phases TetGen runs
  serially (batched predicate evaluation, BRIO-Hilbert sort, point location,
  independent-set point insertion, quality histograms), routed through **NumPP's
  weak-linked `CapabilityRegistry` + `GpuVTable`**, always with a CPU fallback that
  the device path matches within documented tolerance.
- **Python and Swift bindings** as first-class deliverables — NumPy-interoperable
  arrays (via NumPP `.npy`/DLPack) for Python, an idiomatic value-type API for Swift
  (iOS/macOS), each a thin layer over the same C++ core.
- **Exact predicates preserved as a correctness invariant** — Shewchuk adaptive
  predicates are ported verbatim and isolated so the optimizer cannot break them.

## Conventions worth knowing

- **Build system**: CMake ≥ 3.25, C++20, Conan + vcpkg packaging — mirrors
  NumPP / SciPP. Backend feature flags follow NumPP: `CMG_WITH_CUDA`,
  `CMG_WITH_OPENCL`, `CMG_WITH_METAL`, plus integration flags `CMG_WITH_NUMPP`,
  `CMG_WITH_SCIPP`, and binding flags `CMG_WITH_PYTHON`, `CMG_WITH_SWIFT` — all
  default OFF.
- **Task runner**: a `justfile` is the canonical developer entrypoint (mirrors
  NumPP / SciPP) — `just bootstrap`, `just build`, `just test`, `just ctest`,
  `just asan`, `just oracle`, `just spec`, `just ci`, `just clean`. Plain CMake
  still works underneath.
- **Namespacing**: implementation in `src/<capability>/`, public headers in
  `include/cmg/<capability>/`, umbrella header `cmg/cmg.hpp`.
- **Predicate isolation**: `predicates.cpp` is compiled in its own translation
  unit at `-O0` (as TetGen does) so adaptive error analysis stays valid; the rest
  builds at `-O3`. This is a correctness invariant, not an optimization.
- **Oracle breadcrumbs**: spec requirements cite the TetGen source they port,
  e.g. `(oracle: tetgen.cxx:3266)`, `(oracle: predicates.cxx)`, `(oracle: manual §5.2.1)`.

## Working with these specs

- The `bootstrap-cybermesh-foundation` change specifies the **foundation** (data
  model, build/packaging, NumPP/SciPP integration, backend acceleration, robust
  predicates, language bindings, TetGen oracle) in detail and records the full
  **tetgen-parity** roadmap as the scope guardrail. Each meshing capability
  graduates into its own OpenSpec change when picked up.
- New work flows through `propose → apply → validate → archive`.
- Validate with `openspec validate --all --strict`.

## Tracking deferred work and bugs

- **Deferred features** live as an OpenSpec backlog change under
  `openspec/changes/add-*`; the roadmap in `tetgen-parity` is the single source of
  ordering. When an item is picked up it graduates into its own focused,
  oracle-tested change.
- **Bugs** are filed as issues; the fix change MUST add a regression test
  reproducing the bug (per project rules), validated against the TetGen oracle.
- A deferral discovered while implementing is added to the relevant backlog
  `tasks.md` and the change's `proposal.md` Non-goals in the same PR — never left
  only in code comments.

## Licensing note

TetGen 1.6.0 is **AGPLv3** with a WIAS commercial dual-license. A clean-room
port's license must be settled before distribution — see the open question in the
foundation design. This is a legal decision for the maintainer, tracked as a
blocking item, not an engineering default.
