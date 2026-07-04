# voxelization Specification

## Purpose
TBD - created by archiving change add-voxelization. Update Purpose after archive.
## Requirements
### Requirement: Solid voxelization of a PLC into an occupancy grid

CyberMeshGenerator SHALL provide `cmg::voxelize::voxelize(plc, opts)` that produces a
regular axis-aligned grid over the PLC's bounding box (expanded by `opts.pad` margin
cells) with `opts.resolution` cubic cells along the longest axis, marking each cell
inside the closed PLC as occupied. The returned `VoxelGrid` SHALL carry its `origin`,
scalar cubic `spacing`, and `dims (nx, ny, nz)`, and the result SHALL be deterministic
for a given PLC and options. (oracle: none — TetGen has no voxelizer; this is a
grid/raster utility, positioned against B-Rep voxelizers such as OCCT)

#### Scenario: Convex solid fills its interior cells
- GIVEN a closed convex PLC (e.g. a unit cube given by its triangulated boundary)
- WHEN `voxelize(plc, {.resolution = 32, .mode = Occupancy})` is called
- THEN interior cells are marked occupied and the occupied-cell volume approximates the
  domain volume, converging as `resolution` increases

#### Scenario: Deterministic for fixed options
- GIVEN fixed options
- WHEN the same PLC is voxelized twice
- THEN the two grids are identical (same dims, origin, spacing, and cell values)

### Requirement: Exact-predicate inside/outside classification

Voxel occupancy SHALL be decided by exact geometric predicates (a winding-number / ray-
parity test built on the ported Shewchuk `orient3d`), NOT by the carve's fixed-epsilon
ray test, so that for a watertight PLC the classification is robust at the boundary and
on grazing/coplanar configurations, including in single precision. (oracle: robust-
geometric-predicates)

#### Scenario: Boundary cells classify robustly on a watertight mesh
- GIVEN a watertight triangle-mesh PLC with faces grazing the grid lines
- WHEN it is voxelized
- THEN cells are classified consistently (no speckle of misclassified boundary cells),
  and the occupied set matches the mesh interior within one cell of the surface

#### Scenario: Fidelity is bounded by the input tessellation
- GIVEN a curved shape supplied as a triangulated PLC (its exact surface is not available)
- WHEN it is voxelized
- THEN the occupied set conforms to the *triangulated* boundary, not an exact surface —
  accuracy improves with finer input triangulation, and this bound is documented (a
  B-Rep/CAD voxelizer such as OCCT, which classifies against the exact surface, is the
  tool for exact-surface fidelity)

### Requirement: Signed-distance field mode

CyberMeshGenerator SHALL, when `opts.mode == SignedDistance`, populate each cell with the
signed distance from the cell center to the nearest PLC boundary triangle — negative
inside the solid, positive outside — using the boundary-triangle spatial index and the
exact-predicate inside/outside test for the sign. (oracle: none — grid/raster utility)

#### Scenario: Signed distance is zero-crossing at the surface and negative inside
- GIVEN a closed PLC voxelized with `mode = SignedDistance`
- WHEN cell values are inspected
- THEN cells deep inside are negative, cells outside are positive, and the zero level set
  tracks the boundary to within a cell

