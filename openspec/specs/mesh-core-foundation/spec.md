# mesh-core-foundation Specification

## Purpose
TBD - created by archiving change bootstrap-cybermesh-foundation. Update Purpose after archive.
## Requirements
### Requirement: Modern C++20 value-type data model

CyberMeshGenerator SHALL expose a typed, RAII data model in the `cmg` namespace —
at minimum `Point3`, `Mesh` (owning vertex/tetrahedron/face containers), `PLC`
(Piecewise-Linear Complex input with facets, holes, regions), and `MeshOptions`
(typed meshing parameters) — replacing TetGen's raw-array `tetgenio` and
switch-field `tetgenbehavior`. Public entry points SHALL be
`std::expected<Mesh, MeshError> tetrahedralize(const PLC&, const MeshOptions&)` and
`std::expected<Mesh, MeshError> delaunay(std::span<const Point3>, const MeshOptions&)`.
Containers SHALL own their storage (`std::vector`) and be safe to move; no caller
SHALL be required to manage raw `new[]`/`delete[]` arrays. (oracle: tetgen.h `tetgenio`,
`tetgenbehavior`)

#### Scenario: Tetrahedralize a PLC through the typed API
- GIVEN a `PLC` populated with points, facets, and holes
- WHEN `tetrahedralize(plc, MeshOptions{.plc = true})` is called
- THEN it returns a `Mesh` whose points, tetrahedra, and boundary faces are
  populated, with no raw-array bookkeeping exposed to the caller

#### Scenario: Delaunay of a point set
- GIVEN a `std::span<const Point3>`
- WHEN `delaunay(points, {})` is called
- THEN it returns the Delaunay tetrahedralization as a `Mesh`

#### Scenario: Move semantics, no manual free
- GIVEN a returned `Mesh`
- WHEN it is moved and later destroyed
- THEN all storage is released by RAII with no caller-visible allocation calls

### Requirement: Typed options with optional TetGen-switch compatibility

`MeshOptions` SHALL express each meshing parameter as a typed, defaulted field
(e.g. `plc`, `quality{radius_edge, min_dihedral}`, `max_volume`, `preserve_surface`,
`index_base`, `predicate_mode`) rather than a character switch. A compatibility
constructor `MeshOptions::from_switches(std::string_view)` SHALL parse a
TetGen-style switch string (without leading dash) into the typed options, returning
`std::expected<MeshOptions, ParseError>`, so existing TetGen command strings remain
usable. (oracle: tetgen.cxx `parse_commandline` 3074–3795)

#### Scenario: Typed option construction
- WHEN a caller sets `MeshOptions{.plc = true, .quality = Quality{1.414}, .max_volume = 0.1}`
- THEN the options are equivalent to TetGen's `-pq1.414a0.1`

#### Scenario: Switch-string compatibility
- WHEN `MeshOptions::from_switches("pq1.414a0.1")` is called
- THEN it returns options with `plc` true, quality radius-edge `1.414`, and max
  volume `0.1`

#### Scenario: Invalid switch reported, not crashed
- WHEN `from_switches` receives an incompatible combination (e.g. weighted with PLC)
- THEN it returns a `ParseError` describing the conflict rather than aborting

### Requirement: Portable CPU build with no extra dependencies

CyberMeshGenerator's core SHALL build, link, and pass its CPU test subset on any
C++20 toolchain — including iOS and Android cross-compilation toolchains — with all
optional backend and integration flags OFF, requiring only the C++ standard
library. NumPP, SciPP, and the GPU SDKs SHALL NOT be required to build the core.

#### Scenario: No-accel, no-integration build is fully functional
- GIVEN a build with `CMG_WITH_NUMPP=OFF`, `CMG_WITH_SCIPP=OFF` and all
  `CMG_WITH_<GPU>=OFF`
- WHEN the library is built and the CPU test subset is run
- THEN the build succeeds and the CPU tests pass with a built-in spatial index

#### Scenario: Mobile toolchain build
- GIVEN the iOS and Android NDK cross-compilation toolchains
- WHEN the core is built with all backend and integration flags OFF
- THEN compilation and linking succeed

### Requirement: CMake C++20 project layout and feature flags

CyberMeshGenerator SHALL be a CMake (≥ 3.25) C++20 project that surfaces feature
flags in a generated `config.hpp`: backend flags `CMG_WITH_CUDA`,
`CMG_WITH_OPENCL`, `CMG_WITH_METAL`; integration flags `CMG_WITH_NUMPP`,
`CMG_WITH_SCIPP`; binding flags `CMG_WITH_PYTHON`, `CMG_WITH_SWIFT`; and a
precision flag `CMG_SINGLE` — all defaulting OFF. Enabling a GPU flag SHALL imply
`CMG_WITH_NUMPP` and enable the matching NumPP backend flag so the two layers share
one device runtime; configuration SHALL fail fast if the resolved NumPP package
lacks the requested backend. (oracle: tetgen.h `#define REAL double`, `-DSINGLE`)

#### Scenario: Flags default off
- WHEN the project is configured with no options specified
- THEN all `CMG_WITH_*` flags and `CMG_SINGLE` are OFF in the generated config

#### Scenario: GPU flag propagates to NumPP
- WHEN CyberMeshGenerator is configured with `CMG_WITH_CUDA=ON`
- THEN `CMG_WITH_NUMPP` is enabled, NumPP is configured with its CUDA backend, and
  both layers resolve the same device runtime

#### Scenario: Single precision selectable for mobile
- WHEN the project is configured with `CMG_SINGLE=ON`
- THEN the real number type is `float` throughout, mirroring TetGen's `-DSINGLE`

### Requirement: Packaging mirrors the sibling ports

CyberMeshGenerator SHALL ship Conan (`conanfile.py`) and vcpkg (`vcpkg.json`)
packaging that declares NumPP and SciPP as **optional** dependencies (pulled only
when the corresponding integration flag is on), mirroring the NumPP and SciPP
packaging layout. A consumer SHALL be able to depend on the core without pulling
NumPP/SciPP.

#### Scenario: Core consumed without NumPP
- GIVEN a downstream project depending on the CyberMeshGenerator core package with
  no integration flags
- WHEN it resolves dependencies via Conan or vcpkg
- THEN neither NumPP nor SciPP is pulled in

#### Scenario: Accelerated consumer obtains NumPP
- GIVEN a downstream project depending on the package with `CMG_WITH_CUDA=ON`
- WHEN it resolves dependencies
- THEN a NumPP variant built with CUDA is pulled in transitively at the pinned version

### Requirement: Error and result model

CyberMeshGenerator SHALL report recoverable meshing failures (invalid PLC,
self-intersection, quality not met within the Steiner budget, unsupported option
combination) through `std::expected<…, MeshError>` return values, and reserve
`cmg::error` exceptions for programmer errors and violated invariants. It SHALL
NOT terminate the process, call `exit()`, or `longjmp` out of a meshing call, in
contrast to TetGen's `terminatetetgen()` model. When integrated with NumPP,
`cmg::error` SHALL be catchable alongside `numpp::error` at a common base.
(oracle: tetgen.cxx `terminatetetgen`)

#### Scenario: Invalid input returns an error value
- GIVEN a PLC with a self-intersecting facet
- WHEN `tetrahedralize` is called without self-intersection handling enabled
- THEN it returns a `MeshError` describing the self-intersection, and the process
  is not terminated

#### Scenario: Quality unmet within budget reports partial result
- GIVEN options with a Steiner-point budget too small to meet the quality bound
- WHEN `tetrahedralize` returns
- THEN it returns the best mesh achieved together with a status indicating the bound
  was not fully met, without throwing

#### Scenario: Common catch site with NumPP
- GIVEN a build with `CMG_WITH_NUMPP=ON`
- WHEN either layer raises
- THEN a single `catch` at the shared error base handles both

### Requirement: No global mutable state; concurrency-safe calls

CyberMeshGenerator SHALL hold all per-mesh working state in a per-call context
object, with no global mutable meshing state, so that independent `tetrahedralize`
/ `delaunay` calls may run concurrently on different threads without interference.
The one-time predicate initialization SHALL be the only process-global setup and
SHALL be thread-safe. (oracle: tetgen.cxx global `tetgenmesh` state)

#### Scenario: Concurrent independent meshes
- GIVEN two different PLCs
- WHEN `tetrahedralize` is called on each from two threads simultaneously
- THEN both produce correct, independent meshes with no shared-state corruption

