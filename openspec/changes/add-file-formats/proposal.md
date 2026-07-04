# Add mesh file formats (Phase 2)

## Why

The Delaunay kernel (Phase 1) works on in-memory `Point3` arrays, but there is no
way to read input from disk or write results, and no way to feed the same input to
the TetGen oracle and diff the output. File I/O unblocks both real usage and the
live TetGen comparison the oracle harness was designed for. This change ports
TetGen's ASCII geometry/mesh formats and the common interchange formats.

## What changes

Spec delta for the **file-formats** capability — a self-contained I/O layer
(`cmg::io`) over the core `Point3` / `PLC` / `Mesh` types:

- **TetGen native containers** — `.node` (points), `.poly` / `.smesh` (PLC),
  `.ele` (tetrahedra), `.face` (faces), `.edge` (edges), `.neigh` (adjacency),
  read and written, honoring the shared conventions: `#` comments, 0- or 1-based
  indexing (auto-detected on read), whitespace/comma-separated fields, point
  attributes and boundary markers.
- **Interchange formats** — STL (ASCII + binary, with coincident-vertex merge),
  OFF (Geomview), PLY (ASCII), legacy VTK, and Medit `.mesh`, each read as a PLC
  boundary and written from a `Mesh`/`PLC`.
- **Extension dispatch** — `cmg::io::read_plc` / `read_points` / `read_mesh` /
  `write_mesh` / `write_plc` select the format from the file extension (or an
  explicit `Format`), returning `cmg::expected<…, MeshError>` and never throwing
  across the boundary.
- **Round-trip fidelity** — writing then reading a mesh/PLC yields equivalent data
  (points, connectivity, markers) within the format's precision.

## Impact

- Enables the **live TetGen oracle**: a test can write a `.node`/`.poly`, run
  TetGen on it, read back TetGen's `.ele`/`.face`, and diff against
  CyberMeshGenerator's mesh (completes a Phase-1 deferral).
- No change to the meshing kernels; the I/O layer is additive and optional (its
  own `cmg::io` translation units).

## Non-goals

- **No** constraint/sizing files — `.vol`, `.var`, `.mtr` are deferred to the
  phases that consume them (reconstruction / adaptive-sizing).
- **No** Voronoi output files (`.v.node`/`.v.edge`/`.v.face`/`.v.cell`) — Phase 9.
- **No** Gambit `.neu`, and **no** second-order (`-o2`) or `-nn` face/edge
  adjacency columns — those need capabilities not yet built; deferred.
- **No** binary PLY / binary VTK — ASCII PLY and legacy ASCII VTK only this change
  (binary STL is included because STL is commonly binary).
- **No** new meshing behavior — this is purely serialization.
