# Add surface simplification (input decimation) to the core and bindings

## Why

The Bunny and Eiffel examples reduce their input surfaces (112k → 7k triangles,
140k → 7k) with a hand-written Rossignac–Borrel vertex-clustering helper duplicated
in each example. That is a genuine capability — decimating a triangulated PLC surface
before meshing — that belongs in the library, so it is discoverable through the
bindings and not re-implemented per example. (This is distinct from `cmg::coarsen`,
which removes interior vertices from an already-built **tetrahedral** mesh.)

## What changes

New **surface-simplification** capability plus a **language-bindings** delta:

- **Core**: `cmg::simplify::simplify(const PLC&, SimplifyOptions)` → `PLC` — vertex
  clustering on a uniform grid (`grid` cells along the longest axis). Coincident and
  near-coincident vertices in a cell collapse to their centroid; degenerate and
  duplicate triangles are dropped. Deterministic for a given `grid`.
- **C ABI**: `cmg_plc_simplify(const cmg_plc*, int grid, cmg_plc** out, …)`.
- **Python**: `cybermesh.simplify(plc, grid=…)` → `PLC`.
- **Swift**: `CyberMesh.simplify(_ plc:, grid:)` → `PLC`.

## Impact

- The examples drop their duplicated `cluster_decimate` and call `cm.simplify(plc, grid)`.
- Users get explicit control of input resolution / output size in one call.

## Non-goals
- **Quality-preserving decimation** (quadric error metrics / edge collapse) — this is
  the cheap, deterministic grid-clustering method only.
- **Guaranteeing watertightness** — clustering can pinch thin features; the caller
  chooses `grid`. (For the carve, see the companion `accelerate-plc-carve` change,
  which removes the *need* to decimate for tractability.)
