"""Verify cybermesh.simplify: reduces a PLC surface, is deterministic and monotone,
and the result is still meshable. Run with CMG_C_LIB pointed at libcmg_c.so."""
import numpy as np

import cybermesh as cm


def enclosed_volume(P, F):
    a, b, c = P[F[:, 0]], P[F[:, 1]], P[F[:, 2]]
    return abs(np.einsum("ij,ij->i", a, np.cross(b, c)).sum() / 6.0)


def uv_sphere(nlat=24, nlon=24, r=1.0):
    p = cm.PLC()
    pts = [[0, 0, r]]
    ring = []
    for i in range(1, nlat):
        th = np.pi * i / nlat
        row = []
        for j in range(nlon):
            ph = 2 * np.pi * j / nlon
            row.append(len(pts))
            pts.append([r * np.sin(th) * np.cos(ph),
                        r * np.sin(th) * np.sin(ph), r * np.cos(th)])
        ring.append(row)
    bot = len(pts)
    pts.append([0, 0, -r])
    p.add_points(pts)
    for j in range(nlon):
        p.add_facet([0, ring[0][j], ring[0][(j + 1) % nlon]])
    for i in range(nlat - 2):
        for j in range(nlon):
            j1 = (j + 1) % nlon
            p.add_facet([ring[i][j], ring[i + 1][j], ring[i + 1][j1]])
            p.add_facet([ring[i][j], ring[i + 1][j1], ring[i][j1]])
    for j in range(nlon):
        p.add_facet([ring[-1][j], bot, ring[-1][(j + 1) % nlon]])
    return p


def main():
    s = uv_sphere()
    n0 = s.triangles.shape[0]
    v0 = enclosed_volume(s.points, s.triangles)

    coarse = cm.simplify(s, grid=8)
    fine = cm.simplify(s, grid=24)
    assert coarse.triangles.shape[0] < n0, (coarse.triangles.shape[0], n0)
    assert coarse.triangles.shape[0] <= fine.triangles.shape[0]  # monotone

    # shape preserved
    vc = enclosed_volume(coarse.points, coarse.triangles)
    assert abs(vc - v0) / v0 < 0.2, (vc, v0)

    # deterministic
    a, b = cm.simplify(s, grid=12), cm.simplify(s, grid=12)
    assert np.array_equal(a.points, b.points)
    assert np.array_equal(a.triangles, b.triangles)

    # still meshable
    m = cm.tetrahedralize(cm.simplify(s, grid=12), cm.MeshOptions(plc=True))
    assert m.tetrahedra.shape[0] > 0

    print(f"PASS: simplify {n0}->{coarse.triangles.shape[0]} tris (grid 8), "
          f"deterministic, monotone, meshable ({m.tetrahedra.shape[0]} tets)")


if __name__ == "__main__":
    main()
