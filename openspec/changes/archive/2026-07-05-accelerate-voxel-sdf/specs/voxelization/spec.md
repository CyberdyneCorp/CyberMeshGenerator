# voxelization Specification (SDF-acceleration delta)

## MODIFIED Requirements

### Requirement: Signed-distance field mode

CyberMeshGenerator SHALL, when `opts.mode == SignedDistance`, populate each cell with the
signed distance from the cell center to the nearest PLC boundary triangle — negative
inside the solid, positive outside — using the exact-predicate inside/outside test for the
sign. The nearest-triangle distance query SHALL use a spatial acceleration structure over
the boundary triangles (a uniform grid bucketing triangles by their bounding box, searched
by expanding Chebyshev rings around each query cell and cut off once no unsearched cell can
contain a closer triangle), so the distance field's cost scales sub-linearly in
boundary-triangle count rather than testing every triangle for every cell. The computed
distances SHALL be identical to testing every triangle. (oracle: none — grid/raster
utility)

#### Scenario: Signed distance is zero-crossing at the surface and negative inside
- GIVEN a closed PLC voxelized with `mode = SignedDistance`
- WHEN cell values are inspected
- THEN cells deep inside are negative, cells outside are positive, and the zero level set
  tracks the boundary to within a cell

#### Scenario: Accelerated distance equals brute force
- GIVEN a mesh with many boundary triangles voxelized with `mode = SignedDistance`
- WHEN the signed-distance grid is computed
- THEN every cell's distance equals the distance obtained by testing every triangle, and
  the computation uses the spatial index (it does not scale as cells × triangles)
