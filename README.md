# CyberMeshGenerator

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![Spec](https://img.shields.io/badge/spec-OpenSpec-3B5526)](openspec/)

A clean-room **Modern C++20 port of [TetGen](https://codeberg.org/TetGen/TetGen)** —
a Delaunay-based quality tetrahedral mesh generator and 3D Delaunay/Voronoi engine
that runs **everywhere**: pure CPU, GPU desktops/workstations, and mobile
(iOS, Android), with optional **CUDA / OpenCL / Metal** acceleration and
first-class **Python** and **Swift** bindings.

It is the meshing sibling of the CyberdyneCorp scientific stack —
[NumPP](https://github.com/CyberdyneCorp/NumPP) (NumPy port) and
[SciPP](https://github.com/CyberdyneCorp/SciPP) (SciPy port) — and reuses NumPP's
weak-linked device-dispatch substrate rather than building its own.

```cpp
#include "cmg/cmg.hpp"
using namespace cmg;

std::vector<Point3> pts{{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
auto result = delaunay(pts, {});               // -> cmg::expected<Mesh, MeshError>
if (result) {
    const Mesh& m = *result;                   // RAII, owning containers
    // m.tetrahedra, m.faces, m.points ...
}

auto opts = MeshOptions::from_switches("pq1.414a0.1");   // TetGen-compatible
```

## Why it's different from TetGen

- **Modern C++20 API, not char switches** — a typed `MeshOptions` / `PLC` / `Mesh`
  with `std::span`, strong enums and `cmg::expected`, instead of `-pq1.414a0.1`
  strings and raw `tetgenio` arrays. A TetGen switch parser is kept as a
  compatibility layer.
- **Runs everywhere** — one source tree, three build profiles: mobile (CPU-only,
  zero extra deps), desktop, workstation (+GPU). The portable CPU kernel is always
  present and is the only thing required to build.
- **CUDA / OpenCL / Metal** acceleration for the parallelizable phases TetGen runs
  serially, routed through NumPP — always with a CPU fallback the device path
  matches, and identical output topology.
- **Python + Swift bindings** over a stable C ABI, with NumPy interop.
- **Exact predicates preserved** — Shewchuk's adaptive predicates are ported
  verbatim and compiled at `-O0` so the optimizer cannot break their robustness.

## Status

**Phase 1 (Delaunay tetrahedralization) — landed.** On top of the Phase 0
foundation (typed API, robust predicates, build profiles, backend-dispatch and
binding architecture, TetGen oracle harness), the incremental **Bowyer-Watson**
Delaunay kernel is implemented: BRIO-Hilbert-ordered insertion, exact-predicate
point location and cavity, convex-hull faces, optional neighbor adjacency, and the
weighted (regular) DT variant. `delaunay()` now meshes arbitrary point sets.

**Phase 2 (file formats) — landed.** A `cmg::io` layer reads and writes TetGen's
native containers (`.node`, `.poly`, `.smesh`, `.ele`, `.face`, `.edge`, `.neigh`)
and the interchange formats (STL ASCII+binary, OFF, PLY, legacy VTK, Medit
`.mesh`), extension-dispatched, with round-trip fidelity.

This unlocks the **live TetGen oracle**: the suite reads a real TetGen 1.6.0
Delaunay output and confirms `delaunay()` produces the *identical* tetrahedralization
(same sorted-tuple set) on a general-position cloud — the kernel is now validated
against TetGen itself, not only its invariants.

Constrained meshing, quality refinement, sizing, optimization, Voronoi and the CLI
remain scheduled as their own OpenSpec changes — see [`openspec/`](openspec/) for
the roadmap and [`openspec/project.md`](openspec/project.md) for context.

Builds CPU-only with zero dependencies; the suite (66 tests + pure-C ABI smoke
test) is green across the default, `-Werror`, ASan, and single-precision mobile
profiles.

## Build

```bash
just build      # configure + build (portable CPU-only)
just test       # run the foundation test suite
just ctest      # via CTest
just asan       # AddressSanitizer/UBSan
just mobile     # single-precision mobile baseline
just spec       # openspec validate --all --strict
```

Or plain CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tests/cmg_tests
```

### Options (all default OFF — CPU-only is the baseline)

| Flag | Effect |
|------|--------|
| `CMG_WITH_CUDA` / `CMG_WITH_OPENCL` / `CMG_WITH_METAL` | GPU backend (implies `CMG_WITH_NUMPP`) |
| `CMG_WITH_NUMPP` / `CMG_WITH_SCIPP` | Integrate the NumPP/SciPP stack |
| `CMG_WITH_PYTHON` / `CMG_WITH_SWIFT` | Build the language bindings (over the C ABI) |
| `CMG_WITH_THREADS` | Multithreaded CPU acceleration |
| `CMG_SINGLE` | Single-precision (`float`) REAL, for constrained mobile |

## Licensing

TetGen 1.6.0 is **AGPLv3** with a WIAS commercial dual-license. The license of this
port and its provenance discipline (clean-room reimplementation vs. licensed
derivative) is an **open decision for the maintainer** — see
[`NOTICE.md`](NOTICE.md). Do not distribute before it is settled.

## Reference

- Oracle / reference implementation: TetGen 1.6.0 (`/home/leonardo/work/TetGen`).
- Hang Si, "TetGen, a Delaunay-Based Quality Tetrahedral Mesh Generator",
  ACM TOMS 41(2), 2015.
