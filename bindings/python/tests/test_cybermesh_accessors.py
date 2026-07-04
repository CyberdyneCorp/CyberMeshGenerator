"""Verify PLC geometry read-back accessors (points / triangles) and native OBJ/STL
load-and-mesh through the Python binding. Run with CMG_C_LIB pointed at libcmg_c.so."""
import numpy as np

import cybermesh as cm


def tet_volume(m):
    P, T = m.points, m.tetrahedra
    a, b, c, d = P[T[:, 0]], P[T[:, 1]], P[T[:, 2]], P[T[:, 3]]
    return np.abs(np.einsum("ij,ij->i", b - a, np.cross(c - a, d - a))).sum() / 6.0


def cube_plc():
    p = cm.PLC()
    p.add_points([[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0],
                  [0, 0, 1], [1, 0, 1], [1, 1, 1], [0, 1, 1]])
    q = [[0, 3, 2, 1], [4, 5, 6, 7], [0, 1, 5, 4],
         [2, 3, 7, 6], [1, 2, 6, 5], [0, 4, 7, 3]]
    for f in q:
        p.add_facet([f[0], f[1], f[2]])
        p.add_facet([f[0], f[2], f[3]])
    return p


def main():
    p = cube_plc()
    # accessors read the built geometry back
    assert p.points.shape == (8, 3) and p.points.dtype == np.float64, p.points.shape
    assert p.triangles.shape == (12, 3) and p.triangles.dtype == np.int32, p.triangles.shape
    assert p.points.min() == 0.0 and p.points.max() == 1.0

    # native OBJ + STL round-trip: write, read back, geometry matches, mesh is a solid
    for ext in ("obj", "stl", "off"):
        path = f"/tmp/cmg_acc_cube.{ext}"
        cm.write_plc(path, p)                     # raises on failure
        q = cm.read_plc(path)
        assert q.num_points == 8, (ext, q.num_points)
        assert q.triangles.shape[0] == 12, (ext, q.triangles.shape)
        m = cm.tetrahedralize(q, cm.MeshOptions(plc=True))
        assert abs(tet_volume(m) - 1.0) < 1e-6, (ext, tet_volume(m))

    # open-surface path: read vertices back and Delaunay-mesh them
    m2 = cm.delaunay(cm.read_plc("/tmp/cmg_acc_cube.obj").points)
    assert m2.tetrahedra.shape[0] >= 5

    print("PASS: PLC accessors + native OBJ/STL/OFF load-and-mesh")


if __name__ == "__main__":
    main()
