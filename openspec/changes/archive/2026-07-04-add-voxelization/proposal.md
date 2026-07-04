# Add voxelization (occupancy grid + signed-distance field) to the core and bindings

## Why

Users with a surface or PLC often want a **regular 3-D grid** (occupancy or a
signed-distance field) alongside — or instead of — a tetrahedral mesh: for FDTD/FEM on
structured grids, GPU simulation, collision, level sets, or as a sizing field. The core
already owns everything a mesh-native voxelizer needs — the point-in-solid classifier
(`inside_domain`), the boundary-triangle spatial index (`CarveGrid`), and Shewchuk's
exact predicates — so this is a small, high-value addition rather than a new subsystem.

It lives in **CyberMeshGenerator (`cmg`)**, not SciPP: it is built entirely from
cmg-owned geometry and predicates, and its output can feed the existing adaptive-sizing
field. The produced grid is a plain dense-array value type, so SciPP can layer generic
grid operations (morphology, resampling, level-set evolution) on top without this
library depending on SciPP. **Geometry → grid here; grid → grid math in SciPP.**

## What changes

New **voxelization** capability plus a **language-bindings** delta:

- **Core**: `cmg::voxelize::voxelize(const PLC&, VoxelOptions)` → `VoxelGrid`.
  - `VoxelOptions`: `resolution` (cells along the longest bounding-box axis), `pad`
    (margin cells), `mode` (`Occupancy` | `SignedDistance`).
  - `VoxelGrid`: `origin`, cubic `spacing`, `dims (nx, ny, nz)`, and per-cell data
    (occupancy bytes, or signed distances — negative inside).
  - **Classification uses exact geometric predicates** (winding-number / ray-parity via
    `orient3d`), not the carve's epsilon ray test, so a watertight mesh classifies
    robustly at the boundary.
- **C ABI / Python / Swift**: `voxelize(plc, resolution, mode)` returning the grid;
  Python exposes it as a NumPy `(nx, ny, nz)` array plus `origin`/`spacing`.

## Positioning vs OCCT — make the difference explicit

Open CASCADE (OCCT) is a **B-Rep CAD kernel**: it holds *exact analytic* geometry
(NURBS, B-splines) and classifies points against the true surface
(`BRepClass3d_SolidClassifier`), with geometry healing in front. This voxelizer is
different **by representation, not just by tuning**, and the docs/spec state it plainly:

- **Fidelity is bounded by the input tessellation.** CyberMeshGenerator only ever sees a
  PLC (a triangle mesh). It classifies against the boundary *triangles*, so a curved
  input is only as accurate as its faceting. For **STEP/IGES/curved CAD**, OCCT is
  materially better — it voxelizes the exact surface; this library never sees it. This is
  a representational gap and an explicit **non-goal**, not a defect to fix.
- **For a triangle mesh (STL/OBJ/scan)** — this library's native input — there is no
  exact surface for *either* tool, so the quality ceiling is the same. The difference
  reduces to classifier robustness, which this change closes by **requiring exact
  predicates** rather than the epsilon ray test.
- **Where this library is stronger:** speed (a fast spatial-grid classifier, trivially
  parallel → CPU/CUDA/OpenCL/Metal), a tiny footprint (no CAD-kernel dependency), and
  direct integration with meshing and the sizing field.

Summary: this is **fast, exact-predicate, mesh-native voxelization/SDF** — honest that it
is bounded by the input triangulation because it is a mesh engine, not a CAD kernel.

## Impact

- `cybermesh.voxelize(read_plc("part.stl"), resolution=128)` → a NumPy occupancy grid,
  or a signed-distance field, in one call — no CAD dependency.
- The grid can be reused as a background sizing field for meshing.

## Non-goals
- **B-Rep / exact-surface (CAD) voxelization** — that is OCCT's domain; this library has
  no analytic geometry and will not gain one.
- **Hexahedral mesh generation**, **marching-cubes / iso-surface extraction**, and
  **adaptive octree meshing** — separate efforts; this produces a grid, not a new mesh.
- **Conservative / N-separating GPU rasterization semantics** — the classification is
  cell-center inside/outside (plus SDF), not surface-voxel coverage rules.
- **Healing non-watertight input** — best-effort on open/self-intersecting meshes, with
  the limitation documented (no B-Rep healing like OCCT's).
