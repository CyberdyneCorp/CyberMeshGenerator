# language-bindings Specification

## Purpose
TBD - created by archiving change bootstrap-cybermesh-foundation. Update Purpose after archive.
## Requirements
### Requirement: Stable C ABI shim

CyberMeshGenerator SHALL provide a stable C ABI shim (`bindings/c/`) exposing the
core meshing entry points (build a PLC, set options, tetrahedralize, read back
mesh arrays, free) through `extern "C"` functions with opaque handles, so that
language bindings do not depend on C++ name mangling or the C++ ABI. Both the
Python and Swift bindings SHALL sit on this shim rather than on the C++ types
directly. The shim SHALL report failures through return codes plus an
out-error-message parameter, never by throwing across the boundary.

#### Scenario: Bindings depend only on the C ABI
- WHEN the Python or Swift binding is built
- THEN it links against the C ABI shim symbols and does not reference mangled C++
  symbols

#### Scenario: Error crosses the boundary as a code
- GIVEN an invalid PLC passed through the C ABI
- WHEN tetrahedralization is requested
- THEN the shim returns a non-zero error code and fills the error-message buffer,
  without a C++ exception escaping the boundary

### Requirement: Python bindings with NumPy interoperability

CyberMeshGenerator SHALL provide a Python module (`bindings/python/`, built with
`CMG_WITH_PYTHON=ON`) exposing `PLC`, `Mesh`, `MeshOptions`, and `tetrahedralize` /
`delaunay`. Point, tetrahedron, and face arrays SHALL cross the boundary as
**NumPy-interoperable** buffers so that `numpy.asarray(mesh.points)` yields an
`(N, 3)` array without copying — via NumPP's `.npy`/DLPack interop when
`CMG_WITH_NUMPP` is on, otherwise via the Python buffer protocol. The module SHALL
be packaged as an installable wheel (`pyproject.toml`, scikit-build-core).

#### Scenario: NumPy round-trip
- GIVEN a NumPy `(N, 3)` array of points in Python
- WHEN it is passed to `delaunay` and the resulting `mesh.points` is read back with
  `numpy.asarray`
- THEN the input points are accepted without copying and the output is a NumPy array

#### Scenario: Options from Python
- WHEN a Python caller constructs `MeshOptions(plc=True, quality=1.414, max_volume=0.1)`
- THEN it maps to the same typed options as the C++ `MeshOptions`

#### Scenario: Installable wheel
- WHEN the Python package is built
- THEN it produces a wheel that installs and imports on CPython without a separate
  C++ toolchain at install time

### Requirement: Swift bindings for iOS/macOS

CyberMeshGenerator SHALL provide a Swift package (`bindings/swift/`, built with
`CMG_WITH_SWIFT=ON`) exposing idiomatic Swift value types (`PLC`, `Mesh`,
`MeshOptions`) over the C ABI shim, targeting iOS and macOS. On Apple platforms the
package SHALL be able to use the Metal backend when available, falling back to the
CPU path otherwise. It SHALL be distributable as a Swift package / XCFramework.

#### Scenario: Tetrahedralize from Swift
- GIVEN a Swift `PLC` value
- WHEN `tetrahedralize(plc, options)` is called
- THEN it returns a Swift `Mesh` value with points and tetrahedra as Swift arrays

#### Scenario: Metal on device, CPU fallback
- GIVEN an iOS device build with Metal available
- WHEN meshing runs above the offload threshold
- THEN the Metal backend is used; and on a device without a usable GPU the CPU path
  is used and the result is identical topology

### Requirement: Bindings are thin, versioned layers over one core

Each binding SHALL be a thin translation layer that adds no meshing logic of its
own; all algorithms live in the C++ core reached through the C ABI. The bindings
SHALL be independently versionable and buildable without forcing the other binding
to be present, and disabling both binding flags SHALL leave the core build
unaffected.

#### Scenario: Core builds without any binding
- GIVEN a build with `CMG_WITH_PYTHON=OFF` and `CMG_WITH_SWIFT=OFF`
- WHEN the library is built
- THEN the core builds and tests pass with no binding artifacts produced

#### Scenario: One binding without the other
- GIVEN a build with `CMG_WITH_PYTHON=ON` and `CMG_WITH_SWIFT=OFF`
- WHEN the project is built
- THEN the Python wheel is produced and no Swift package is required

### Requirement: C ABI covers PLC construction, options, and outputs

The stable C ABI SHALL expose building a PLC (points plus facets via
`cmg_plc_add_facet`, and hole/region seeds), setting the mesh options that the typed
API supports (PLC mode, quality, maximum volume, edge preservation, index base),
`cmg_tetrahedralize`, and reading back the mesh arrays including tetrahedron and face
markers — so a binding can drive the full meshing surface without touching C++.
(oracle: language-bindings; mesh-core-foundation)

#### Scenario: PLC meshed through the C ABI
- GIVEN a PLC built through the C ABI (points + facets) and options with `plc` set
- WHEN `cmg_tetrahedralize` is called
- THEN a mesh handle is returned whose tetrahedron and face arrays are readable

### Requirement: Python meshing with NumPy interop

The Python module SHALL expose `PLC`, `MeshOptions`, `tetrahedralize`, and `delaunay`
over the C ABI, returning mesh points as an `(N,3)` NumPy array and tetrahedra as an
`(M,4)` NumPy array. Building and meshing a PLC from Python SHALL produce a valid mesh.
(oracle: language-bindings; NumPy interop)

#### Scenario: Cube PLC meshed from Python
- GIVEN a unit-cube `PLC` built in Python and `MeshOptions(plc=True)`
- WHEN `tetrahedralize` is called
- THEN `mesh.points` is an `(N,3)` array, `mesh.tetrahedra` is an `(M,4)` array, and the
  summed tetrahedron volume is ~1.0

