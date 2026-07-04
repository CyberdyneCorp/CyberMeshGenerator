# language-bindings Specification (file I/O delta)

## ADDED Requirements

### Requirement: C ABI file loading and saving

The stable C ABI SHALL expose reading a PLC (`cmg_read_plc`), a point set
(`cmg_read_points`), and a volumetric mesh (`cmg_read_mesh`) from a file, and writing
a mesh (`cmg_write_mesh`) or PLC (`cmg_write_plc`) to a file, with the format inferred
from the path extension and failures returned as status codes plus a message (never a
thrown exception). It SHALL also expose `cmg_plc_num_points`. (oracle: language-bindings;
file-formats)

#### Scenario: Load a surface and mesh it through the C ABI
- GIVEN a surface file (e.g. `.stl` or `.obj`) on disk
- WHEN `cmg_read_plc` loads it and the resulting PLC is passed to `cmg_tetrahedralize`
- THEN a mesh handle is returned, without any format parsing in the caller

#### Scenario: Unreadable file is a clean error
- WHEN `cmg_read_plc` is given a missing or unsupported-extension path
- THEN a non-zero status and an error message are returned, and *out is left NULL

### Requirement: Python file loading and saving

The Python module SHALL provide `read_plc(path)` → `PLC`, `read_points(path)` → `PLC`,
`read_mesh(path)` → `Mesh`, `write_mesh(path, mesh)`, and `write_plc(path, plc)` over
the C ABI, so a model file can be loaded and meshed without an external parser.
(oracle: language-bindings)

#### Scenario: Load an STL and mesh it from Python
- GIVEN a watertight `.stl` surface file
- WHEN `cybermesh.tetrahedralize(cybermesh.read_plc(path), MeshOptions(plc=True))` is
  called
- THEN a valid `Mesh` with NumPy arrays is returned

### Requirement: PLC geometry read-back accessors

The C ABI SHALL expose reading a loaded PLC's geometry back out — its point count
(`cmg_plc_num_points`), points (`cmg_plc_points`, a flat `xyz` array), triangle count
(`cmg_plc_num_triangles`), and its facets fan-triangulated to a flat index array
(`cmg_plc_triangles`). The Python and Swift bindings SHALL surface these as `points` and
`triangles` accessors so a file-loaded PLC's geometry is readable without re-parsing the
source file. (oracle: language-bindings)

#### Scenario: Read a loaded PLC's geometry back
- GIVEN a PLC loaded from a surface file (e.g. an `.obj`)
- WHEN its `points` and `triangles` accessors are read
- THEN they return the vertex coordinates (N×3) and fan-triangulated facet indices (M×3),
  and the points can be passed straight to `delaunay` for an open-surface mesh

### Requirement: Swift file loading and meshing

The Swift binding SHALL provide `CyberMesh.readPLC(path)` → `PLC`,
`CyberMesh.readMesh(path)` → `Mesh`, `CyberMesh.tetrahedralize(plc, options)`,
`CyberMesh.delaunay(points, options)`, and `PLC.points` / `PLC.triangles` accessors over
the same C ABI, mirroring the Python surface so a model file can be loaded and meshed from
Swift without an external parser. (oracle: language-bindings)

#### Scenario: Load a surface and mesh it from Swift
- GIVEN a surface file (`.stl` / `.obj` / `.off` / `.ply`) on disk
- WHEN `CyberMesh.tetrahedralize(try CyberMesh.readPLC(path), opts)` is called with an
  options value whose `plc` flag is set
- THEN a valid `Mesh` value is returned, with failures surfaced as a thrown `MeshError`
