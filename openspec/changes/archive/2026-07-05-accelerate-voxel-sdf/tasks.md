# Tasks — Accelerate signed-distance voxelization

- [x] Build a uniform 3-D grid bucketing boundary triangles by their AABB (reuse the voxel grid geometry)
- [x] Nearest-triangle distance per cell via expanding Chebyshev-ring search with a `(r-1)·spacing ≥ best` cutoff and a per-query visited stamp (no double-counting)
- [x] Route `fill_distance` through the index instead of the brute-force triangle loop
- [x] Confirm the SDF is identical to brute force (unit cube/sphere distances unchanged; existing SDF test passes)
- [x] Add a test that a denser mesh's SDF matches a brute-force reference within tolerance and completes quickly
- [x] Measure the speedup vs brute force on a real mesh (Eiffel/bunny) and record it
- [x] Example: the Eiffel SDF panel can use less pre-decimation
- [x] `openspec validate --all --strict` green; suite green across default / -Werror / ASan / single precision / clang++

## Deferred
- [ ] GPU-accelerated SDF; narrow-band / approximate SDF; BVH
