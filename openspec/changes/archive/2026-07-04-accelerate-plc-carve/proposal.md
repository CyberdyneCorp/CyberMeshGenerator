# Accelerate the PLC interior carve with a spatial index

## Why

The boundary-conforming carve classifies each candidate tetrahedron by ray-casting its
centroid against **every** boundary triangle (`inside_domain` in
`src/constrained/tetrahedralize_plc.cpp`), so the carve costs `O(num_tets ×
num_triangles)`. That is why the Bunny/Eiffel examples must decimate their surfaces to a
few thousand triangles before meshing — a full-resolution 112k-triangle surface is
billions of ray/triangle tests. The decimation is a workaround for this scaling wall,
not a modelling need.

## What changes

Spec delta for **constrained-tetrahedralization** (performance, behaviour-preserving):

- The point-in-domain test SHALL use a spatial acceleration structure over the boundary
  triangles so a query tests only triangles that its ray can cross. Because the carve
  uses a fixed set of ray directions, index each direction by projecting every triangle
  onto the plane orthogonal to that ray into a uniform 2-D grid; a query then tests only
  the triangles in its own grid cell.
- The classification result SHALL be identical to the brute-force test (the cell of a
  query contains every triangle whose projection covers the ray, hence every triangle
  the ray could hit), so meshes are unchanged — only faster.

## Impact

- The carve becomes roughly linear in triangle count; full-resolution closed surfaces
  mesh directly, and example decimation becomes optional (detail/size control via the
  companion `add-surface-simplification` change) rather than required.
- No change to output meshes: Bunny volume and Eiffel vs-TetGen parity are unchanged.

## Non-goals
- **Exact concave boundary recovery / facet insertion** — unchanged; this only speeds
  up the existing centroid classification.
- A general BVH — a per-direction 2-D bucket grid suffices for the fixed ray set.
