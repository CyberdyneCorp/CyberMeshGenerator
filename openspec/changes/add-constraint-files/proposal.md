# Add constraint/sizing file I/O (.vol, .mtr)

## Why

The file-formats layer reads/writes geometry and mesh containers but not TetGen's
constraint files. `.vol` (per-tetrahedron maximum volume) feeds mesh reconstruction/
refinement, and `.mtr` (per-node sizing metric) feeds adaptive sizing — both were
deferred from earlier phases. These are simple ASCII formats and directly unblock
file-driven refinement and sizing.

## What changes

Spec delta for the **file-formats** capability:

- `read_vol(path, num_tets)` / `write_vol(path, vols, base)` — the `.vol` format:
  header `<#tets>`, then `<tet#> <max-volume>` lines; a zero or negative volume means
  "unconstrained". Returns one value per tetrahedron.
- `read_mtr(path)` / `write_mtr(path, sizes)` — the `.mtr` format: header
  `<#nodes> <metric-size=1>`, then one value per node (the target edge length; 0 =
  ignored). Returns one value per node.
- Shared conventions apply (comments, whitespace separation).

## Impact

- A `.mtr` file can be loaded and turned into a `sizing` function (with the Phase 5
  background/analytic sizing); a `.vol` file provides per-tet volume targets for
  reconstruction-driven refinement.

## Non-goals

- **`.var`** (per-facet-area / per-segment-length constraints) — a more complex,
  marker-keyed format; deferred.
- **Anisotropic `.mtr`** (tensor metrics, 6 values/node) — isotropic scalar only.
- Wiring these into the meshing pipeline — this change is the I/O only.
