# Accelerate signed-distance voxelization with a spatial index

## Why

`cmg::voxelize` occupancy mode is already spatially indexed (xy-bucketed, ~O(cells +
triangles)), but the **signed-distance** mode computes each cell's nearest-triangle
distance by brute force — `O(cells × triangles)` in `fill_distance`. That is the wall the
Eiffel example hit: the tower had to be decimated to ~35k triangles just to keep the SDF
tractable, and even then it takes ~10 s. A nearest-triangle spatial index makes the SDF
scale so full-resolution meshes are practical.

## What changes

Spec delta for **voxelization** (performance, behaviour-preserving):

- The nearest-boundary-triangle distance query SHALL use a spatial acceleration structure
  over the boundary triangles (a uniform grid bucketing triangles by their AABB, searched
  by expanding Chebyshev rings around each query cell with a distance-based cutoff), so
  the SDF cost scales sub-linearly in triangle count rather than testing every triangle
  per cell.
- The result SHALL be **identical** to the brute-force distance (the ring search visits a
  superset of the candidate triangles and prunes only cells that provably cannot contain a
  closer triangle), so SDF grids are unchanged — only faster.

## Impact

- SDF at full resolution on large meshes becomes practical (the Eiffel example can drop or
  shrink its pre-decimation for the SDF panel).
- No change to output: existing SDF tests and the sign/zero-crossing behaviour are
  unchanged.

## Non-goals
- **GPU-accelerated SDF** — still deferred; this is the CPU spatial index.
- **Approximate / narrow-band SDF** — the field remains exact everywhere.
- A BVH — a uniform grid with an expanding-ring search suffices for the voxel query
  pattern (queries are the grid cells themselves).
