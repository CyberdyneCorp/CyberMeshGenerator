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
