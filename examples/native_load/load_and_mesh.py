#!/usr/bin/env python3
"""Native file loading — no hand-parsers.

CyberMeshGenerator reads OBJ and STL (and .off/.ply/.poly/.smesh) directly, so a
model goes from file to mesh in a few lines through the `cybermesh` binding:

    import cybermesh as cm
    plc  = cm.read_plc("model.stl")                 # or .obj / .off / .ply / .poly
    mesh = cm.tetrahedralize(plc, cm.MeshOptions(plc=True))   # closed solid
    # or, for an open surface, use the vertices:
    mesh = cm.delaunay(cm.read_plc("model.obj").points)

`plc.points` (N,3 float64) and `plc.triangles` (M,3 int32) read the loaded geometry
back as NumPy arrays.

Run:

    CMG_C_LIB=$(readlink -f "$(find ../../build-py -name libcmg_c.so | head -1)") \
    PYTHONPATH=../../bindings/python/src python3 load_and_mesh.py
"""
from __future__ import annotations

from pathlib import Path

import numpy as np

import cybermesh as cm

HERE = Path(__file__).parent


def tet_volume(m):
    P, T = m.points, m.tetrahedra
    a, b, c, d = P[T[:, 0]], P[T[:, 1]], P[T[:, 2]], P[T[:, 3]]
    return np.abs(np.einsum("ij,ij->i", b - a, np.cross(c - a, d - a))).sum() / 6.0


def demo_closed_solid():
    """Write a cube to OBJ and STL, then load + mesh each natively."""
    plc = cm.PLC()
    plc.add_points([[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0],
                    [0, 0, 1], [1, 0, 1], [1, 1, 1], [0, 1, 1]])
    q = [[0, 3, 2, 1], [4, 5, 6, 7], [0, 1, 5, 4],
         [2, 3, 7, 6], [1, 2, 6, 5], [0, 4, 7, 3]]
    for f in q:
        plc.add_facet([f[0], f[1], f[2]])
        plc.add_facet([f[0], f[2], f[3]])

    for ext in ("obj", "stl"):
        path = str(HERE / f"cube.{ext}")
        cm.write_plc(path, plc)                       # native writer
        loaded = cm.read_plc(path)                    # native reader
        mesh = cm.tetrahedralize(loaded, cm.MeshOptions(plc=True))
        print(f"  cube.{ext}: read_plc -> {loaded.num_points} pts / "
              f"{loaded.triangles.shape[0]} tris -> tetrahedralize -> "
              f"{mesh.tetrahedra.shape[0]} tets, vol={tet_volume(mesh):.4f}")


def demo_open_surface():
    """Load an OBJ surface and Delaunay-mesh its vertices (via .points)."""
    obj = HERE.parent / "antenna" / "Antenna.obj"
    if not obj.exists():
        print("  (antenna OBJ not present — skipping open-surface demo)")
        return
    plc = cm.read_plc(str(obj))                       # native OBJ reader
    verts = plc.points                                # (N, 3) float64, read back
    mesh = cm.delaunay(verts)
    print(f"  {obj.name}: read_plc -> {verts.shape[0]} vertices -> "
          f"delaunay -> {mesh.tetrahedra.shape[0]} tetrahedra")


def main():
    print("Closed solids (read_plc -> tetrahedralize):")
    demo_closed_solid()
    print("Open surface (read_plc.points -> delaunay):")
    demo_open_surface()
    print("All native — no OBJ/STL parsing in Python.")


if __name__ == "__main__":
    main()
