# Add full Python bindings (language-bindings)

## Why

The C ABI shim and a minimal Python module exist, but only expose point-set Delaunay.
The full surface — building a PLC (facets/holes/regions), setting all mesh options, and
reading back every output — is what makes the library usable from Python. Python 3 +
NumPy 2.1 are available, so this is testable end-to-end.

## What changes

Spec delta for the **language-bindings** capability:

- **Extended C ABI** (`bindings/c`): PLC facet construction
  (`cmg_plc_add_facet`), region/hole seeds, MeshOptions setters (`plc`, `quality`,
  `max_volume`, `preserve_edges`, `index_base`), `cmg_tetrahedralize`, and mesh output
  getters for tet/face markers (points/tets/faces already exist).
- **Python module** (`bindings/python`, ctypes over the C ABI): `PLC`, `MeshOptions`,
  `tetrahedralize`, and `delaunay` returning NumPy arrays (`mesh.points` `(N,3)`,
  `mesh.tetrahedra` `(M,4)`, `mesh.faces`), with markers.

## Impact

- From Python: build a cube `PLC`, set `max_volume`, `tetrahedralize`, and read the
  result as NumPy arrays — verified against the mesh volume.

## Non-goals
- **Swift binding completion** — no Swift toolchain in this environment; the C ABI it
  sits on is extended and tested, but the Swift wrapper is source-only/untested here.
- **nanobind compiled extension** — the ctypes path over the C ABI is the tested
  surface; a nanobind wheel is a packaging follow-up.
- **Exposing every capability** (voronoi/quality-report/coarsen) through Python — the
  core meshing surface (PLC/options/mesh) lands here; the rest is incremental.
