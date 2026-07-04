# Stanford Bunny example — watertight STL → **solid** tetrahedral mesh

Loads the Stanford Bunny (`Stanford_Bunny_sample.stl`, a binary STL), tetrahedralizes
its **solid interior** with CyberMeshGenerator through the Python (`cybermesh`)
binding, and renders the surface next to a cutaway of the volumetric mesh.

![comparison](bunny_comparison.png)

- **Left** — the watertight input surface.
- **Right** — the mesh our library produced: `cybermesh.tetrahedralize(plc, ...)`
  carves the **interior** (ray-cast point-in-domain), so the tet mesh boundary
  **conforms to the bunny** (not the convex hull). Shown as a cutaway (the `x <`
  median half's boundary faces) so the interior tetrahedra are visible.

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

## Run it

From this directory, after building the shared C ABI once from the repo root
(`cmake -S . -B build-py -DCMG_BUILD_C_ABI=ON -DCMG_BUILD_SHARED=ON -DCMG_BUILD_TESTS=OFF -DCMG_BUILD_CLI=OFF && cmake --build build-py -j`):

```bash
CMG_C_LIB=$(readlink -f "$(find ../../build-py -name libcmg_c.so | head -1)") \
PYTHONPATH=../../bindings/python/src python3 run_bunny.py
```

Requires `numpy` + `matplotlib` (no GUI — renders offscreen via Agg). Outputs:
`bunny_comparison.png`, `bunny_surface.png`, `bunny_tetmesh.png`.
