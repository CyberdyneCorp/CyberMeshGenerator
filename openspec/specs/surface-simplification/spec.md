# surface-simplification Specification

## Purpose
Decimate a triangulated PLC surface before meshing by grid vertex clustering, so dense
inputs (e.g. high-triangle STL scans) can be reduced to a tractable, still-closed
surface. This is a preprocessing utility on the input surface — distinct from
`mesh-coarsening`, which removes interior vertices from an already-built tetrahedral
mesh — exposed through the C ABI and the Python/Swift bindings.
## Requirements
### Requirement: Grid vertex-clustering simplification of a PLC surface

CyberMeshGenerator SHALL provide `cmg::simplify::simplify(plc, opts)` that reduces a
triangulated PLC surface by Rossignac–Borrel vertex clustering: the bounding box is
divided into `opts.grid` cells along its longest axis, all input vertices falling in a
cell collapse to a single representative at their centroid, and each facet triangle is
re-emitted over the representatives with degenerate (repeated-vertex) and duplicate
triangles dropped. The result SHALL be a `PLC` with no more triangles than the input,
and the reduction SHALL be deterministic for a given `grid`. (oracle: none — TetGen has
no surface simplifier; this is a preprocessing utility)

#### Scenario: Dense surface is reduced, shape preserved
- GIVEN a densely triangulated closed surface (e.g. a >100k-triangle STL loaded as a PLC)
- WHEN `simplify(plc, {.grid = 34})` is called
- THEN a PLC with substantially fewer triangles is returned whose enclosed volume is
  within a few percent of the input's

#### Scenario: Deterministic for a fixed grid
- GIVEN a fixed `grid`
- WHEN the same PLC is simplified twice
- THEN both results are identical (same points and triangles)

#### Scenario: Coarser grid yields fewer triangles
- GIVEN the same PLC simplified at `grid = 20` and `grid = 60`
- THEN the `grid = 20` result has no more triangles than the `grid = 60` result

