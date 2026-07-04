"""End-to-end test for the CyberMesh Python bindings over the C ABI.

Builds a unit-cube PLC (8 corners, 6 quad faces each split into two triangular
facets), tetrahedralizes it with PLC meshing enabled, and checks that the
returned NumPy arrays are well-shaped and that the total tetrahedron volume of
the boundary-conforming mesh equals the cube's volume (1.0).

Run with the freshly built shared library:

    CMG_C_LIB=/path/to/libcmg_c.so python3 test_cybermesh.py
"""
import os
import sys

import numpy as np

# Make the in-tree package importable without installation.
_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(_HERE, "..", "src"))

import cybermesh as cm  # noqa: E402


def cube_plc() -> cm.PLC:
    """Unit cube [0,1]^3 as 8 corners and 12 triangular facets."""
    corners = np.array([
        [0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0],
        [0, 0, 1], [1, 0, 1], [1, 1, 1], [0, 1, 1],
    ], dtype=np.float64)
    # Each face given as an outward quad loop; split into two triangles.
    quads = [
        (0, 3, 2, 1),  # bottom  z=0
        (4, 5, 6, 7),  # top     z=1
        (0, 1, 5, 4),  # y=0
        (2, 3, 7, 6),  # y=1
        (1, 2, 6, 5),  # x=1
        (0, 4, 7, 3),  # x=0
    ]
    plc = cm.PLC()
    plc.add_points(corners)
    for a, b, c, d in quads:
        plc.add_facet((a, b, c), marker=1)
        plc.add_facet((a, c, d), marker=1)
    return plc


def tet_volume_sum(mesh: cm.Mesh) -> float:
    """Sum of absolute tetrahedron volumes via a vectorized scalar triple product."""
    p = mesh.points
    t = mesh.tetrahedra
    a = p[t[:, 0]]
    v1 = p[t[:, 1]] - a
    v2 = p[t[:, 2]] - a
    v3 = p[t[:, 3]] - a
    dets = np.einsum("ij,ij->i", np.cross(v1, v2), v3)
    return float(np.abs(dets).sum() / 6.0)


def main() -> int:
    print(f"cybermesh backend={cm._BACKEND} core version={cm.version()}")

    plc = cube_plc()
    mesh = cm.tetrahedralize(plc, cm.MeshOptions(plc=True))

    print(f"points.shape     = {mesh.points.shape}  dtype={mesh.points.dtype}")
    print(f"tetrahedra.shape = {mesh.tetrahedra.shape}  dtype={mesh.tetrahedra.dtype}")
    print(f"faces.shape      = {mesh.faces.shape}  dtype={mesh.faces.dtype}")

    assert mesh.points.ndim == 2 and mesh.points.shape[1] == 3, mesh.points.shape
    assert mesh.points.dtype == np.float64
    assert mesh.tetrahedra.ndim == 2 and mesh.tetrahedra.shape[1] == 4, \
        mesh.tetrahedra.shape
    assert mesh.tetrahedra.dtype == np.int32
    assert mesh.faces.ndim == 2 and mesh.faces.shape[1] == 3, mesh.faces.shape
    assert mesh.tetrahedra.shape[0] >= 5, "cube needs at least 5 tets"

    # Indices must be in range.
    assert mesh.tetrahedra.min() >= 0
    assert mesh.tetrahedra.max() < mesh.points.shape[0]

    vol = tet_volume_sum(mesh)
    print(f"total tet volume = {vol!r}")
    assert abs(vol - 1.0) < 1e-6, f"volume {vol} != 1.0"

    # Delaunay path still works.
    d = cm.delaunay(np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 1]],
                             dtype=np.float64))
    assert d.tetrahedra.shape == (1, 4), d.tetrahedra.shape
    print(f"delaunay single tet = {d.tetrahedra.shape}")

    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
