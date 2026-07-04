# Design — CyberMeshGenerator foundation

## Context

CyberMeshGenerator ports TetGen to Modern C++20. TetGen is a single ~36k-line
translation unit driven by global mutable state and single-character command
switches, with Shewchuk's exact predicates in a separate `-O0` file. The port
keeps what makes TetGen correct (the algorithms and the exact predicates) and
replaces what makes it hard to reuse, embed, and accelerate (global state, C-style
memory pools, `char*` switches, file-extension-driven control flow).

Two forces shape every decision:

1. **Portability.** The library must build and run CPU-only on iOS/Android with
   zero extra dependencies. Acceleration and bindings are strictly additive.
2. **Robustness.** Delaunay-based meshing is only correct if the geometric
   predicates are exact. That is a hard invariant, preserved by porting
   `predicates.cxx` verbatim and isolating it at `-O0`.

## Goals / non-goals

- **Goal**: one source tree, three build profiles (mobile CPU-only, desktop, GPU
  workstation) from the same code, selecting the fastest available path at runtime.
- **Goal**: every result validatable against real TetGen 1.6.0 (topology + exact
  predicate signs).
- **Goal**: GPU acceleration (CUDA/OpenCL/Metal) for TetGen's parallelizable
  phases, reusing NumPP's device layer — never a second device stack.
- **Goal**: Python and Swift bindings as thin layers over a stable C ABI.
- **Non-goal (this change)**: any Phase 1+ meshing capability. Only the skeleton,
  data model, integration contracts, predicate port, acceleration architecture,
  binding architecture, and oracle harness are in scope.

## Data model — Modern C++20, replacing `tetgenio`/`tetgenbehavior`

TetGen exposes `tetgenio` (raw `REAL*`/`int*` arrays with manual `numberof*`
counts and `new[]`/destructor ownership) and `tetgenbehavior` (100+ bool/int
fields set from switch chars). The port replaces these with value types:

```cpp
namespace cmg {

struct Point3 { double x, y, z; };              // REAL is double by default

// A tetrahedral mesh: owning, RAII, move-only-cheap.
class Mesh {
  std::vector<Point3>            points;        // vertex coordinates
  std::vector<std::array<int,4>> tets;          // tetrahedra (vertex indices)
  std::vector<std::array<int,3>> faces;         // boundary/all faces
  std::vector<int>               tet_markers, face_markers;
  // adjacency (opt-in): neighbors, tet2face, face2tet ...
  // second-order nodes (opt-in, -o2 analogue)
};

// A Piecewise-Linear Complex: the input domain (replaces .poly / facetlist).
class PLC {
  std::vector<Point3>            points;
  std::vector<Facet>            facets;         // facet = list of polygons + holes
  std::vector<Point3>           holes;          // seed points inside holes
  std::vector<Region>           regions;        // x,y,z,attribute,max_volume
};

// Typed options — replaces the `-pq1.414a0.1` switch string.
struct MeshOptions {
  bool                 plc            = false;   // -p
  std::optional<Quality> quality;               // -q: radius_edge, min_dihedral
  std::optional<double>  max_volume;            // -a#
  bool                 preserve_surface = false; // -Y
  IndexBase            index_base     = IndexBase::Zero; // -z
  Predicate            predicate_mode = Predicate::ExactAdaptive; // -X analogue
  // ... one typed field per TetGen switch, discoverable, defaulted.
  static std::expected<MeshOptions, ParseError>
  from_switches(std::string_view);              // optional TetGen-compat parser
};

std::expected<Mesh, MeshError>
tetrahedralize(const PLC& in, const MeshOptions& opts);

std::expected<Mesh, MeshError>
delaunay(std::span<const Point3> points, const MeshOptions& opts);

} // namespace cmg
```

- **Errors**: recoverable failures (invalid PLC, self-intersection, unmet quality
  under Steiner budget) are returned via `std::expected<…, MeshError>`; programmer
  errors and unrecoverable invariants use `cmg::error` exceptions. This replaces
  TetGen's `terminatetetgen()`/`longjmp` + exit-code model.
- **No global state.** All working state lives in a per-call context object; two
  `tetrahedralize` calls can run concurrently on different threads.

## Optional NumPP / SciPP integration

NumPP and SciPP are **optional** (`CMG_WITH_NUMPP`, `CMG_WITH_SCIPP`, default OFF).
The core geometry types above have zero external dependencies so the mobile
profile stays clean.

- **When `CMG_WITH_NUMPP=ON`**: (a) the point/tet arrays gain zero-copy adapters
  to/from `numpp::ndarray` (`Mesh::points_view() -> ndarray`,
  `PLC::from_ndarray(...)`) for I/O and Python interop; and (b) the accelerated
  kernels route through NumPP's `CapabilityRegistry`/`GpuVTable` (see
  backend-acceleration). NumPP is consumed as a **pinned Conan/vcpkg release
  dependency** resolved via `find_package`, not vendored.
- **When `CMG_WITH_SCIPP=ON`**: point location and sizing may use
  `scipp::spatial::KDTree` and distance kernels instead of the built-in locator,
  reusing already-accelerated code.
- **When both OFF**: the mesher is fully functional with a built-in CPU spatial
  index; NumPP/SciPP contribute acceleration and interop, never correctness.

GPU flag propagation: enabling `CMG_WITH_CUDA` requires/implies `NUMPP_WITH_CUDA`
(and hence `CMG_WITH_NUMPP`), so both layers share one device runtime; configure
fails fast if the resolved NumPP package lacks the requested backend.

```
              cmg::tetrahedralize / delaunay / refine / optimize / voronoi
                                   │  (PLC in, Mesh out)
                                   ▼
             cmg core: predicates · BRIO-Hilbert sort · Bowyer-Watson · CDT
                                   │  (optional accel path)
                                   ▼
   numpp::CapabilityRegistry · weak GpuVTable · last_backend()   ← reused, not rebuilt
                                   │
   portable CPU kernel ──► CUDA / OpenCL / Metal (runtime select)
```

## Module / repo layout (mirrors NumPP / SciPP)

```
include/cmg/
  cmg.hpp                # umbrella header
  fwd.hpp                # forward decls
  version.hpp(.in)
  core/                  # Point3, Mesh, PLC, MeshOptions, error model
  predicates/            # exact/adaptive predicate interface
  backend/               # dispatch shims over NumPP's registry
  delaunay/  constrained/  quality/  sizing/  optimize/
  coarsen/   reconstruct/ region/    voronoi/  io/   cli/
src/<capability>/        # implementation .cpp mirroring include/
src/predicates/predicates.cpp   # ported verbatim, built -O0 (own TU)
bindings/
  python/                # NumPy-interop module (nanobind), pyproject.toml
  swift/                 # Swift package (iOS/macOS) over the C ABI shim
  c/                     # stable C ABI shim both bindings sit on
tests/                   # Catch2 + TetGen oracle harness, golden/ frozen data
cmake/  conanfile.py  vcpkg.json  CMakeLists.txt  justfile
```

Reserved-but-empty capability dirs make the phased roadmap visible in the tree;
each is filled by its own later change.

## Robust predicates — ported verbatim, isolated

- `predicates.cxx` is transcribed to `src/predicates/predicates.cpp` with minimal
  change (namespacing, `REAL` typedef), **not** rewritten. `exactinit()` runs once
  at startup to compute machine-epsilon error bounds.
- The predicate TU is compiled at **`-O0`** while the rest builds at `-O3`
  (CMake `set_source_files_properties(... COMPILE_OPTIONS -O0)`), exactly as
  TetGen's makefile does (`PREDCXXFLAGS = -O0`). This prevents FP-contraction /
  reassociation from breaking the adaptive error analysis. **Correctness
  invariant, not an optimization.**
- The public interface is `orient3d`/`insphere`/`orient2d`/`incircle` plus a
  **batched** form (`orient3d_batch(spans...)`) that the accelerated point-location
  and insertion phases call; the batch form is what a GPU backend can evaluate over
  many candidate simplices at once (fast filter on device, exact escalation on CPU
  for the uncertain-sign minority).

## Backend acceleration — reusing NumPP, not rebuilding it

TetGen is serial. Several of its phases are data-parallel and are the ones we
accelerate, **registering kernels into NumPP's existing dispatch shape** rather
than creating a parallel device stack:

- **Accelerable phases**: batched predicate/filter evaluation, BRIO-Hilbert
  spatial sort (Hilbert-curve key computation is embarrassingly parallel), spatial
  point location, independent-set / non-conflicting parallel point insertion, and
  quality-histogram / worst-tet scans.
- **What we reuse unchanged**: `CapabilityRegistry` (compiled-backend + present-
  device probing), `last_backend()`, the `NUMPP_GPU_TARGET` override
  (`cpu|cuda|opencl|metal|auto`), and the bounded device buffer reuse pool.
- **What CyberMeshGenerator adds**: mesh-shaped kernels behind the same weak-linked
  vtable idea, each gated by its `CMG_WITH_<BACKEND>` flag, built as a separate
  weak-linked TU, **null/absent** when not compiled or when no device is present.
- **Dispatch rule** (inherited): choose from `(op, problem size, available
  backends)`; below a per-op size threshold use the CPU kernel; a CPU fallback
  always exists; on Apple `auto` prefers Metal, else CUDA → OpenCL → CPU.
- **The serial core stays correct regardless.** The GPU accelerates *candidate
  generation and filtering*; the topological mutation and the exact-sign decisions
  remain on a deterministic path so the output is combinatorially identical to the
  CPU build (within documented tie-breaking), not merely "close".

Inherently sequential/branchy phases (segment recovery, flip cascades) stay
CPU-only by design.

## Language bindings

Both bindings sit on a **stable C ABI shim** (`bindings/c/`) so neither depends on
C++ name mangling and each can version independently.

- **Python** (`bindings/python/`, nanobind): exposes `PLC`, `Mesh`, `MeshOptions`
  and `tetrahedralize`/`delaunay`. Point/tet arrays cross the boundary as
  **NumPy-interoperable** buffers — via NumPP's `.npy`/DLPack when `CMG_WITH_NUMPP`
  is on, otherwise via the buffer protocol — so `numpy.asarray(mesh.points)` is
  zero-copy. Packaged as a wheel (`pyproject.toml`, scikit-build-core).
- **Swift** (`bindings/swift/`, SwiftPM): an idiomatic value-type API
  (`struct PLC`, `struct Mesh`, `Rotation`-style options) wrapping the C ABI,
  targeting iOS and macOS (Metal-accelerated on device). Distributed as a Swift
  package / XCFramework.
- This change specifies the **architecture and the C ABI contract**; full binding
  surface completion is Phase 11.

## TetGen oracle harness

Mirrors NumPP's NumPy oracle and SciPP's SciPy oracle:

- A test states inputs and options; the harness runs real TetGen 1.6.0
  (`/home/leonardo/work/TetGen`) on the equivalent switch string and compares:
  **exact predicate signs** must be identical; **DT/CDT topology** must be
  combinatorially equivalent (same set of tetrahedra as sorted vertex tuples in
  general position); **counts, quality histograms, and Steiner points** match
  within documented tie-breaking.
- A **frozen/checked mode** serializes reference meshes into `tests/golden/` so CI
  runs without a TetGen build.
- **Degenerate inputs** (cospherical / coplanar points) are compared under the
  same symbolic-perturbation tie-breaking TetGen uses; where insertion order makes
  topology non-unique, the harness asserts validity + Delaunay-property invariants
  rather than exact-tuple equality, documented per case.
- Breadcrumbs in specs cite the TetGen source ported, e.g. `(oracle: tetgen.cxx:3266)`.

## Phasing rationale

Predicates + `delaunay` go first because every other capability is built on the DT
kernel and correct predicates. `file-formats` comes second so later capabilities
have real inputs and a way to diff outputs against TetGen. CDT, quality, and sizing
follow in dependency order; optimization/coarsening/reconstruction, then Voronoi
and the optional CLI, then GPU device kernels + binding completion + v1.0.

## Resolved decisions

- **NumPP/SciPP are optional, pinned package dependencies** (not vendored, not
  hard deps). Core builds CPU-only with zero extra deps; enabling a GPU flag pulls
  a NumPP variant built with the matching backend, else configure fails fast.
- **Predicates ported verbatim at `-O0`**, not rewritten — robustness invariant.
- **Bindings sit on a C ABI shim**, not directly on C++, so Python and Swift
  version independently and avoid mangling/ABI coupling.
- **Default real type is `double`** (as TetGen); a `CMG_SINGLE` option selects
  `float` for memory-constrained mobile, mirroring TetGen's `-DSINGLE`.

## Open questions (blocking before distribution)

- **License.** TetGen 1.6.0 is AGPLv3 with a WIAS commercial dual-license. A
  clean-room reimplementation of the *algorithms* from the published paper +
  behavioral spec may avoid AGPL obligations, but transcribing `tetgen.cxx` or
  `predicates.cxx` inherits their license. **The maintainer must decide** the
  license and the provenance discipline (clean-room reimplementation vs. licensed
  derivative) before any public release. This is a legal decision, not an
  engineering default; flagged here rather than silently assumed.
- **Predicate provenance.** Shewchuk's predicates carry their own permissive
  public-domain-ish terms distinct from TetGen's AGPL; confirm the exact terms of
  the version bundled in `predicates.cxx` before shipping.
