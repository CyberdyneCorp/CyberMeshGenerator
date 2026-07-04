# Tasks — Voxelization

- [ ] Core types: `cmg::voxelize::VoxelOptions` (resolution, pad, mode) and `VoxelGrid` (origin, spacing, dims, data)
- [ ] Exact-predicate inside/outside test (winding-number / ray-parity via `orient3d`), distinct from the carve's epsilon ray test
- [ ] `cmg::voxelize::voxelize(PLC, opts)` — occupancy mode over the boundary-triangle spatial index; deterministic
- [ ] Signed-distance mode: nearest-boundary-triangle distance + exact-predicate sign
- [ ] CMake: add `src/voxelize/voxelize.cpp`; add to `cmg/cmg.hpp`
- [ ] Core tests: cube occupancy volume converges with resolution; determinism; watertight boundary robustness (incl. single precision); SDF sign + zero-crossing
- [ ] C ABI: `cmg_plc_voxelize` returning grid data + origin/spacing/dims
- [ ] Python: `cybermesh.voxelize(plc, resolution, mode)` → NumPy `(nx,ny,nz)` + origin/spacing
- [ ] Swift: `CyberMesh.voxelize(_ plc:, resolution:, mode:)` (source-only; no Swift toolchain here)
- [ ] Python test: voxelize a loaded PLC; occupancy volume sane; sdf sign correct
- [ ] Docs: README note positioning vs OCCT (mesh-native, tessellation-bounded, exact-predicate)
- [ ] `openspec validate --all --strict` green; suite green across default / -Werror / ASan / single precision

## Deferred
- [ ] B-Rep / exact-surface (CAD) voxelization; hex meshing; marching-cubes / iso-surfacing; octree meshing
- [ ] Conservative / N-separating GPU rasterization semantics; healing of non-watertight input
- [ ] GPU-accelerated classification kernel (CPU first; grid is embarrassingly parallel)
