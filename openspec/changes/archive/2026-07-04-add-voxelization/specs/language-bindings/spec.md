# language-bindings Specification (voxelization delta)

## ADDED Requirements

### Requirement: Voxelization exposed through the bindings

The C ABI SHALL expose voxelizing a PLC into a grid (`cmg_plc_voxelize` with a resolution
and a mode selector, returning the grid data plus `origin`, `spacing`, and `dims`), and
the Python and Swift bindings SHALL expose `voxelize(plc, resolution, mode)` over it.
Python SHALL return the grid as a NumPy `(nx, ny, nz)` array (occupancy or signed
distance) together with its `origin` and `spacing`. Failures SHALL cross the C boundary
as status codes plus a message, never as a thrown exception. (oracle: language-bindings)

#### Scenario: Voxelize a loaded PLC from Python
- GIVEN a PLC loaded from a surface file
- WHEN `cybermesh.voxelize(plc, resolution=64)` is called
- THEN a NumPy occupancy grid of shape `(nx, ny, nz)` is returned with its `origin` and
  `spacing`, and requesting `mode="sdf"` returns a float signed-distance grid instead

#### Scenario: Invalid resolution is a clean error
- WHEN `cmg_plc_voxelize` is given a resolution less than 1
- THEN a non-zero status and an error message are returned and no grid is produced
