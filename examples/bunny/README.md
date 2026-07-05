# Stanford Bunny example — watertight STL → **solid** tetrahedral mesh

Loads the Stanford Bunny (`Stanford_Bunny_sample.stl`, a binary STL) natively via
`cybermesh.read_plc` — no hand-written STL unpacking — and simplifies it in the library
with `cybermesh.simplify` (no hand-written vertex clustering). It then tetrahedralizes
the **solid interior** with CyberMeshGenerator through the Python (`cybermesh`) binding,
and renders the surface next to a cutaway of the volumetric mesh. (The carve is now
spatially indexed, so the full 112k-triangle surface meshes directly too — simplifying
just keeps the render light.)

![comparison](bunny_comparison.png)

- **Left** — the watertight input surface.
- **Middle** — the mesh our library produced (`cybermesh.tetrahedralize(plc, ...)`):
  the **full** solid, whose boundary reconstructs the complete bunny — nothing is
  missing.
- **Right** — the **same** mesh shown as a *cutaway* (the `x <` median half's
  boundary faces) so the interior tetrahedra are visible. This cut is a *view*, not
  the mesh: it is the reason a naïve glance might look "half" a bunny.

## Correctness check (it really fills the solid)

The example self-validates: the solid tet-mesh volume is compared against the volume
**enclosed by the input surface** (divergence theorem — the ground truth):

```
volume check: mesh=276532  surface-enclosed=275900  (100.23% — the solid is complete)
```

i.e. our carve fills the bunny interior to **~0.2 %** of the true volume (the match
holds across resolutions — e.g. grid 48: 278222 vs 278216, ~0.002 %).

### Cross-check vs TetGen

Running real **TetGen** (`tetgen -p`) on the *same* decimated surface is instructive:
TetGen **refuses** it — vertex-cluster decimation introduces a handful of
self-intersecting triangles, and TetGen aborts ("input surface mesh contains
self-intersections") after emitting only the *un-carved convex hull* (volume 424668,
= the DT-of-vertices upper bound). CyberMeshGenerator's ray-cast carve is more
tolerant of the imperfect input and still returns the correct interior volume. (Our
own `cmg::detect::self_intersections` would likewise flag those triangles.) A
production pipeline would decimate with a manifold-preserving simplifier so both
tools accept the surface.

This is the counterpart to the [antenna example](../antenna): the antenna is an
**open** truss surface (no closed interior → `delaunay` of the vertices), whereas the
bunny is **closed and watertight** → the true solid interior is meshed.

## Decimation

The full STL is **112,402 triangles** — too many for the ray-cast carve
(`O(tets · triangles)`). It is first **vertex-cluster decimated** (Rossignac-Borrel:
bin corners into a grid, average each cell, re-emit unique triangles) to a
still-closed ~6.7k-triangle surface, then meshed to **~11k tetrahedra in ~4 s**. Raise
`GRID` in `run_bunny.py` for a finer mesh (slower). The tet mesh is cached to
`_bunny_cache.npz` (gitignored) for fast re-renders.

> A production pipeline would decimate with a quality-preserving simplifier and feed
> the mesh through `cybermesh.tetrahedralize` with a `max_volume` for graded
> refinement; this example keeps its dependencies to `numpy` + `matplotlib`.

## Voxelization — vs `trimesh`

The example also **voxelizes** the bunny with `cybermesh.voxelize` (exact-predicate solid
occupancy) and, if [`trimesh`](https://trimesh.org) is installed, compares against its
solid voxelizer (`mesh.voxelized(pitch).fill()`) resampled onto the same grid:

![voxelization](bunny_voxelization.png)

```
our voxels (res 48): 24667 occupied, volume 280129 (100.2% of enclosed)
vs trimesh: IoU=0.854 Dice=0.921 cell-agree=96.0%  trimesh volume 328007 (117.3% of enclosed)
```

- **Left** — our voxels; **middle** — trimesh's; **right** — agreement (gray = both,
  blue = ours only, orange = trimesh only).
- Both agree on ~96 % of cells (**IoU 0.85**). The difference is a **boundary shell**
  (the orange layer): our exact vertical-ray-parity classifier marks the *interior*, so
  the occupied volume matches the divergence-theorem enclosed volume to **~0.2 %**, while
  trimesh's surface-voxels-plus-fill adds a one-cell surface shell and over-counts by
  ~17 %. Neither is "wrong" — they answer slightly different questions (interior vs
  surface-inclusive); our number is the one that conserves the true solid volume.
- This uses the **full 112k-triangle watertight** surface directly (voxelization needs no
  decimation, and the exact classifier is only guaranteed on watertight input — a hole
  would let a column's parity leak into a spurious spike).

## Run it

From this directory, after building the shared C ABI once from the repo root
(`cmake -S . -B build-py -DCMG_BUILD_C_ABI=ON -DCMG_BUILD_SHARED=ON -DCMG_BUILD_TESTS=OFF -DCMG_BUILD_CLI=OFF && cmake --build build-py -j`):

```bash
CMG_C_LIB=$(readlink -f "$(find ../../build-py -name libcmg_c.so | head -1)") \
PYTHONPATH=../../bindings/python/src python3 run_bunny.py
```

Requires `numpy` + `matplotlib` (no GUI — renders offscreen via Agg); `trimesh` is
optional and only enables the voxelization comparison (skipped with a note if absent).
Outputs: `bunny_comparison.png`, `bunny_surface.png`, `bunny_tetmesh.png`, and
`bunny_voxelization.png`.
