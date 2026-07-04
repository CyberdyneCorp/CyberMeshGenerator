# Native file loading

CyberMeshGenerator reads and writes OBJ, STL, OFF, PLY, and its native TetGen
formats directly, so the bindings go from a file on disk to a mesh with **no
hand-written parser**. This example shows both meshing paths through the Python
binding.

```python
import cybermesh as cm

# Closed solid: load a surface PLC and tetrahedralize its interior.
plc  = cm.read_plc("model.stl")                          # or .obj / .off / .ply
mesh = cm.tetrahedralize(plc, cm.MeshOptions(plc=True))

# Open surface: read the vertices back and Delaunay-mesh them.
mesh = cm.delaunay(cm.read_plc("model.obj").points)
```

The loaded PLC exposes its geometry as NumPy arrays:

| accessor          | shape / dtype     | meaning                              |
|-------------------|-------------------|--------------------------------------|
| `plc.points`      | `(N, 3)` float64  | vertex coordinates                   |
| `plc.triangles`   | `(M, 3)` int32    | facets, fan-triangulated to indices  |
| `plc.num_points`  | `int`             | vertex count                         |

## Run

```bash
CMG_C_LIB=$(readlink -f "$(find ../../build-py -name libcmg_c.so | head -1)") \
PYTHONPATH=../../bindings/python/src python3 load_and_mesh.py
```

Expected output:

```
Closed solids (read_plc -> tetrahedralize):
  cube.obj: read_plc -> 8 pts / 12 tris -> tetrahedralize -> 6 tets, vol=1.0000
  cube.stl: read_plc -> 8 pts / 12 tris -> tetrahedralize -> 6 tets, vol=1.0000
Open surface (read_plc.points -> delaunay):
  Antenna.obj: read_plc -> 8832 vertices -> delaunay -> 62405 tetrahedra
All native — no OBJ/STL parsing in Python.
```

(The open-surface line is skipped if `examples/antenna/Antenna.obj` is not present.)

## Swift

The Swift binding mirrors this surface over the same C ABI —
`CyberMesh.readPLC(path)`, `CyberMesh.tetrahedralize(plc, opts)`,
`CyberMesh.delaunay(points)`, and `PLC.points` / `PLC.triangles`. See
[`bindings/swift/Sources/CyberMesh/CyberMesh.swift`](../../bindings/swift/Sources/CyberMesh/CyberMesh.swift).
