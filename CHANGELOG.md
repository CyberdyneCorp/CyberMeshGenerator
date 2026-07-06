# Changelog

All notable changes to CyberMeshGenerator are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html) (pre-1.0: minor/patch may still carry
breaking changes while the API stabilizes).

## [Unreleased]

## [0.5.0] - 2026-07-06

### Added
- **Installable package** — `install`/`export` rules and a generated
  `CyberMeshGeneratorConfig.cmake` so downstream projects can
  `find_package(CyberMeshGenerator)` and link `cmg::cmg` (and `cmg::c` when the C ABI is built).
  The core library is CPU-only and self-contained; optional NumPP/SciPP/Threads dependencies are
  re-resolved transitively by the config via `find_dependency`. The C ABI shared library carries a
  semver `SOVERSION`.
- **Consumability spec** — a portable OpenSpec "usable by others" readiness rubric
  (`openspec/specs/consumability`) consolidating the install/packaging/CI/versioning/governance
  requirements, reusable as an adoption checklist for other projects.
- **Project governance** — `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, this changelog,
  and GitHub issue/PR templates.
- **`just gpu-detect`** — a cross-platform recipe that probes for CUDA / OpenCL / Metal and
  recommends a build recipe.
- **Versioning & API stability** documentation in the README.

### Changed
- Synced the package-manifest versions (`conanfile.py`, `vcpkg.json`) with the CMake project
  version, which had drifted.

## [0.4.0]

Baseline before the production-readiness work. A clean-room Modern C++20 port of TetGen:
Delaunay/constrained tetrahedralization, quality refinement, adaptive sizing, region/hole
classification, Voronoi/power diagrams, mesh optimization, simplification, coarsening,
reconstruction, and exact-predicate voxelization; TetGen-compatible switch parser; a broad set of
file formats (STL / OBJ / OFF / PLY / `.poly` / `.node` / `.ele` / `.face` / `.vtk` / `.mesh`);
a CLI, a stable C ABI, and Python (ctypes/NumPy) + Swift bindings; optional CUDA backend
(OpenCL/Metal scaffolded) and optional NumPP/SciPP integration. Validated against TetGen 1.6.0 as
a test oracle.
