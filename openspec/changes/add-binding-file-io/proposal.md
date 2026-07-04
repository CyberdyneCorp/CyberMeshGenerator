# Add file loading/saving to the C ABI and Python bindings (language-bindings)

## Why

The `cmg::io` layer can load STL/OBJ/OFF/PLY/native meshes, but the bindings can't —
so the Python examples hand-parse STL/OBJ. Exposing file I/O through the C ABI lets a
binding load a model in one call (`cybermesh.read_plc("bunny.stl")`), which is how
users actually want to feed real data in.

## What changes

Spec delta for the **language-bindings** capability:

- **C ABI**: `cmg_read_plc` (surface → PLC: `.poly/.smesh/.stl/.obj/.off/.ply`),
  `cmg_read_points` (`.node`), `cmg_read_mesh` (`.ele`+`.node`/`.vtk`/`.mesh`),
  `cmg_write_mesh`, `cmg_write_plc`, and `cmg_plc_num_points`. Format is inferred
  from the extension; errors cross the boundary as codes + a message.
- **Python**: `cybermesh.read_plc(path)` → `PLC`, `read_mesh(path)` → `Mesh`,
  `read_points(path)` → `PLC`, `write_mesh(path, mesh)`, `write_plc(path, plc)`.

## Impact

- `mesh = cybermesh.tetrahedralize(cybermesh.read_plc("bunny.stl"), opts)` — no
  external parser. The examples can drop their hand-written STL/OBJ readers.

## Non-goals
- **New formats** — this exposes the existing `cmg::io` set (OBJ is added by the
  companion change); it adds no parsers of its own.
- **Streaming / partial loads**; **format-specific options** (e.g. STL merge
  tolerance) — defaults only.
