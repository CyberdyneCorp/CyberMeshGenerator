"""Round-trip file I/O test for the CyberMesh Python bindings over the C ABI.

Builds a unit-cube PLC in Python, writes it to a ``.off`` surface file via
``write_plc``, reads it back with ``read_plc``, tetrahedralizes the recovered
boundary, and checks that the total tetrahedron volume equals the cube's (1.0).

``.off`` is used for the round-trip because it is a fully working reader/writer,
so the test does not depend on the concurrently-developed OBJ reader.

Run with the freshly built shared library:

    CMG_C_LIB=/path/to/libcmg_c.so python3 test_cybermesh_io.py
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

    path = "/tmp/cmg_cube.off"

    # Write the cube boundary out, then read it back in from disk.
    plc = cube_plc()
    cm.write_plc(path, plc)
    assert os.path.exists(path), path

    loaded = cm.read_plc(path)
    assert isinstance(loaded, cm.PLC)
    npts = loaded.num_points
    print(f"read_plc num_points = {npts}")
    assert npts == 8, f"expected 8 cube corners, got {npts}"

    # Tetrahedralize the boundary recovered from disk and check the volume.
    mesh = cm.tetrahedralize(loaded, cm.MeshOptions(plc=True))
    assert mesh.tetrahedra.shape[0] >= 5, "cube needs at least 5 tets"
    assert mesh.tetrahedra.min() >= 0
    assert mesh.tetrahedra.max() < mesh.points.shape[0]

    vol = tet_volume_sum(mesh)
    print(f"total tet volume = {vol!r}")
    assert abs(vol - 1.0) < 1e-6, f"volume {vol} != 1.0"

    print(f"PASS volume={vol}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
