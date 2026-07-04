# Add Wavefront OBJ read/write (file-formats)

## Why

The file-formats layer reads STL/OFF/PLY/VTK/Medit surfaces, but not **Wavefront
OBJ** — one of the most common 3-D interchange formats (the antenna example had to
hand-parse it in Python). Adding OBJ lets `cmg::io` load `.obj` surfaces directly as
a PLC, on the same footing as the other interchange formats.

## What changes

Spec delta for the **file-formats** capability:

- `read_obj(path)` → `PLC`: parse a Wavefront OBJ — vertex (`v`), optional texture
  (`vt`) and normal (`vn`) lines are read; faces (`f`, with `v`/`v/vt`/`v/vt/vn`
  or `v//vn` corner syntax and any polygon size) become facets (fan-triangulated
  where a downstream triangle is needed, but stored as the polygon). Only the
  geometry is used; materials (`mtllib`/`usemtl`) are skipped.
- `write_obj(path, plc)` / `write_obj_mesh(path, mesh)`: write the vertices (`v`)
  and the facets / boundary faces (`f`) as an OBJ surface.
- Extension dispatch: `.obj` → `read_plc` / `write_plc` / `write_mesh`.

## Impact

- `cmg::io::read_plc("model.obj")` returns a PLC; `write_plc("out.obj", plc)` writes
  one — the antenna-style OBJ workflow no longer needs an external parser.

## Non-goals
- **Materials / textures** (`.mtl`, `vt`/`vn` semantics) — geometry only; texture
  coordinates are parsed but not attached to the PLC.
- **Negative / relative indices** beyond the standard 1-based positive form.
- **Smoothing groups, curves, free-form geometry.**
