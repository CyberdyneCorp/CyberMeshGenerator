# Antenna example — textured OBJ → tetrahedral mesh

Loads a textured Wavefront OBJ surface (`Antenna.obj` + `Antenna.mtl` +
`Antenna.jpg`, a lattice antenna tower), tetrahedralizes its vertices with
**CyberMeshGenerator** through the Python (`cybermesh`) binding, and renders the
original textured surface next to the volumetric tetrahedral mesh.

![comparison](antenna_comparison.png)

- **Left** — the original surface (8,832 vertices · 14,852 triangles), per-face
  colored by sampling the JPG texture at each triangle's UV and Lambert-shaded.
- **Right** — the mesh our library produced: the Delaunay tetrahedralization of the
  surface vertices (**62,405 tetrahedra**), shown as a cutaway (the lower-z half's
  boundary faces) so the interior tetrahedra are visible.

> **Loaded natively.** The surface is loaded with `cybermesh.read_plc("Antenna.obj")`
> — no hand-parser. Its geometry is read back through the accessors: `plc.points`
> (8,832 × 3) feeds `cybermesh.delaunay`, and `plc.triangles` (fan-triangulated
> facets) drives the surface render. The only thing still read in Python is the
> *texture* channel (`vt` UVs), which the loader drops by design — a small helper
> parses just those to color the faces, fan-triangulated to line up 1:1 with
> `plc.triangles`.

The result is a genuine tetrahedralization of the real 8.8k-vertex point set (~19 s
on one CPU core; cached to `_tetmesh_cache.npz` for fast re-renders).

> This example uses `delaunay` (fill the convex hull of the vertices) because the
> antenna OBJ is an open, non-watertight truss surface — there is no closed solid
> interior to carve. For a **closed** PLC, use `cybermesh.tetrahedralize(plc, opts)`
> to mesh the domain interior (see `bindings/python`).

## Run it

From this directory, after building the shared C ABI once from the repo root:

```bash
# repo root — build the shared C ABI the Python binding loads
cmake -S . -B build-py -DCMG_BUILD_C_ABI=ON -DCMG_BUILD_SHARED=ON \
      -DCMG_BUILD_TESTS=OFF -DCMG_BUILD_CLI=OFF && cmake --build build-py -j

# this directory — generate the PNGs
CMG_C_LIB=$(find ../../build-py -name libcmg_c.so | head -1) \
PYTHONPATH=../../bindings/python/src python3 run_antenna.py
```

Requirements: `numpy`, `matplotlib`, `Pillow` (all pip-installable); no GUI needed
(renders offscreen via matplotlib's Agg backend). Set `CMG_NO_CACHE=1` to force
re-tetrahedralization.

Outputs: `antenna_comparison.png`, `antenna_original.png`, `antenna_tetmesh.png`.
