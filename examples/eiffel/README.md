# Eiffel Tower example — CyberMeshGenerator **vs TetGen**, head-to-head

Tetrahedralizes the vertices of an Eiffel Tower STL with both **CyberMeshGenerator**
and **real TetGen**, and compares the results. The STL is loaded natively with
`cybermesh.read_plc`, and TetGen's own output (`.ele` + `.node`) is read back with
`cybermesh.read_mesh` — both sides go through the native loaders, no hand-parsing.

![comparison](eiffel_comparison.png)

- **Left** — the Eiffel surface (an **open lattice** — a truss, not a watertight
  solid, so there is no closed interior to carve; decimated 139,989 → 7,292 tris).
- **Middle** — CyberMeshGenerator's Delaunay tetrahedralization of the vertices.
- **Right** — **real TetGen**'s Delaunay of the *same* points.

The middle and right panels are identical because the two meshes **are** identical:

```
OURS   Delaunay: 12326 tets, vol=117802, 19 ms
TETGEN Delaunay: 12326 tets, vol=117802, 13 ms
COMPARE        : identical tetrahedra 12326/12326 = 100.00%  | Δvolume = 1.46e-11
```

Every one of the 12,326 tetrahedra matches TetGen's (as sorted vertex tuples), and
the total volume agrees to machine precision — on a real 3D model whose clustered
points contain many coplanar/cospherical configurations. This is the strongest
validation of the Delaunay engine: **bit-for-bit agreement with the reference
implementation** it is a port of. (A Delaunay tetrahedralization fills the *convex
hull* of the points — the frustum shape in the middle/right — so both tools produce
the same hull-filling mesh, cut away here to show the interior.)

> The Eiffel STL is ~35 MB and is **not vendored**. Point `EIFFEL_STL` at your copy.
> `TETGEN_BIN` is optional — without it the TetGen panel is skipped and only
> CyberMeshGenerator runs.

## Run it

After building the shared C ABI (see the repo root):

```bash
EIFFEL_STL=/path/to/Eiffel_tower_sample.STL \
TETGEN_BIN=/path/to/tetgen \
CMG_C_LIB=$(readlink -f "$(find ../../build-py -name libcmg_c.so | head -1)") \
PYTHONPATH=../../bindings/python/src python3 run_eiffel.py
```

Requires `numpy` + `matplotlib` (no GUI — renders offscreen via Agg).
