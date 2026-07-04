#!/usr/bin/env python3
"""Stanford Bunny example — load a watertight STL, tetrahedralize its SOLID
interior with CyberMeshGenerator, and render the surface next to the volumetric
tetrahedral mesh (cutaway).

Unlike the antenna (an open truss surface), the bunny is a closed watertight mesh,
so this uses `cybermesh.tetrahedralize(plc, ...)` — the interior is carved out, and
the tet mesh boundary CONFORMS to the bunny shape (not the convex hull).

The full STL is 112k triangles — too many for the ray-cast carve — so it is first
vertex-cluster decimated to a tractable, still-closed surface, then meshed.

Run (headless; writes PNGs):

    CMG_C_LIB=$(find ../../build-py -name libcmg_c.so | head -1) \
    PYTHONPATH=../../bindings/python/src python3 run_bunny.py

Requires: numpy, matplotlib (Pillow not needed — the bunny STL has no texture).
"""
from __future__ import annotations

import os
import struct
import time
from collections import Counter
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

import cybermesh as cm

HERE = Path(__file__).parent
STL = HERE / "Stanford_Bunny_sample.stl"
GRID = 34  # vertex-clustering resolution for meshing (higher = finer, slower)


def read_binary_stl(path: Path):
    """Return the (T, 3, 3) triangle-corner array from a binary STL."""
    d = path.read_bytes()
    n = struct.unpack("<I", d[80:84])[0]
    rec = np.frombuffer(d[84:84 + 50 * n], dtype=np.uint8).reshape(n, 50)
    fl = np.frombuffer(rec[:, :48].tobytes(), dtype="<f4").reshape(n, 4, 3)
    return fl[:, 1:4, :].astype(np.float64)  # drop the per-facet normal


def cluster_decimate(corners, grid=GRID):
    """Rossignac-Borrel vertex clustering: bin corners into a grid, average each
    cell to a representative vertex, and re-emit non-degenerate unique triangles.
    Keeps the surface closed while cutting the triangle count ~20x."""
    V = corners.reshape(-1, 3)
    lo, ext = V.min(0), (V.max(0) - V.min(0)).max()
    cell = np.floor((V - lo) / ext * grid).astype(int)
    key = cell[:, 0] * grid * grid + cell[:, 1] * grid + cell[:, 2]
    uk, inv = np.unique(key, return_inverse=True)
    reps = np.zeros((len(uk), 3))
    cnt = np.zeros(len(uk))
    np.add.at(reps, inv, V)
    np.add.at(cnt, inv, 1)
    reps /= cnt[:, None]
    vi = inv.reshape(len(corners), 3)
    faces, seen = [], set()
    for a, b, c in vi:
        if a == b or b == c or a == c:
            continue
        k = tuple(sorted((int(a), int(b), int(c))))
        if k not in seen:
            seen.add(k)
            faces.append((int(a), int(b), int(c)))
    return reps, faces


def set_equal(ax, V):
    lo, hi = V.min(0), V.max(0)
    ax.set_xlim(lo[0], hi[0]); ax.set_ylim(lo[1], hi[1]); ax.set_zlim(lo[2], hi[2])
    ax.set_box_aspect(hi - lo)
    ax.set_axis_off()


def shade(polys, base=(0.80, 0.72, 0.60), light=(0.5, 0.6, 0.6)):
    n = np.cross(polys[:, 1] - polys[:, 0], polys[:, 2] - polys[:, 0])
    n /= (np.linalg.norm(n, axis=1, keepdims=True) + 1e-12)
    ld = np.asarray(light) / np.linalg.norm(light)
    lam = 0.30 + 0.70 * np.abs(n @ ld)
    return np.clip(np.asarray(base)[None, :] * lam[:, None], 0, 1)


def render_surface(ax, P, F):
    polys = P[np.asarray(F)]
    pc = Poly3DCollection(polys, facecolors=shade(polys),
                          edgecolors=(0, 0, 0, 0.08), linewidths=0.1)
    pc.set_rasterized(True)
    ax.add_collection3d(pc)
    set_equal(ax, P)
    ax.set_title(f"Watertight input surface\n{len(P)} verts · {len(F)} triangles",
                 fontsize=10)


def render_tetmesh(ax, mesh):
    """Cutaway: keep the x < median half of the tetrahedra, draw that half's
    boundary faces — the cut plane reveals interior tetrahedra and the bunny-
    conforming boundary."""
    P, T = mesh.points, mesh.tetrahedra
    cx = P[T].mean(axis=1)[:, 0]
    kept = T[cx < np.median(cx)]
    fc, face_of = Counter(), {}
    for t in kept:
        for f in ((t[1], t[2], t[3]), (t[0], t[2], t[3]),
                  (t[0], t[1], t[3]), (t[0], t[1], t[2])):
            k = tuple(sorted(f)); fc[k] += 1; face_of[k] = f
    bfaces = [face_of[k] for k, m in fc.items() if m == 1]
    polys = P[np.asarray(bfaces)]
    depth = polys.mean(axis=1)[:, 0]
    colors = plt.cm.plasma((depth - depth.min()) / (np.ptp(depth) + 1e-9))
    pc = Poly3DCollection(polys, facecolors=colors, edgecolors=(0, 0, 0, 0.2),
                          linewidths=0.15)
    pc.set_rasterized(True)
    ax.add_collection3d(pc)
    set_equal(ax, P)
    ax.set_title("CyberMeshGenerator solid tet mesh (cutaway)\n"
                 f"{len(T)} tetrahedra · interior conforms to the bunny",
                 fontsize=10)


def main():
    print("Reading", STL.name, "...")
    corners = read_binary_stl(STL)
    print(f"  {len(corners)} triangles (full)")
    P, F = cluster_decimate(corners)
    print(f"  decimated (grid {GRID}): {len(P)} verts, {len(F)} triangles")

    cache = HERE / "_bunny_cache.npz"
    if cache.exists() and os.environ.get("CMG_NO_CACHE") is None:
        d = np.load(cache)
        mesh = cm.Mesh(points=d["points"], tetrahedra=d["tets"],
                       faces=np.empty((0, 3), np.int32),
                       tet_markers=np.empty((0,), np.int32),
                       face_markers=np.empty((0,), np.int32))
        print(f"  loaded cached tet mesh: {mesh.tetrahedra.shape[0]} tetrahedra")
    else:
        print("Tetrahedralizing the SOLID interior (PLC carve)...")
        plc = cm.PLC(); plc.add_points(P)
        for f in F:
            plc.add_facet(list(f), marker=1)
        t0 = time.time()
        mesh = cm.tetrahedralize(plc, cm.MeshOptions(plc=True))
        print(f"  {mesh.tetrahedra.shape[0]} tetrahedra in {time.time() - t0:.1f}s")
        np.savez_compressed(cache, points=mesh.points, tets=mesh.tetrahedra)

    fig = plt.figure(figsize=(13, 6))
    for i, fn in enumerate((lambda ax: render_surface(ax, P, F),
                            lambda ax: render_tetmesh(ax, mesh))):
        ax = fig.add_subplot(1, 2, i + 1, projection="3d")
        ax.view_init(elev=18, azim=130)
        fn(ax)
    fig.suptitle("CyberMeshGenerator — Stanford Bunny (solid interior mesh)",
                 fontsize=13)
    fig.tight_layout()
    fig.savefig(HERE / "bunny_comparison.png", dpi=130, bbox_inches="tight")
    print("Wrote", HERE / "bunny_comparison.png")

    for name, fn in (("bunny_surface.png", lambda ax: render_surface(ax, P, F)),
                     ("bunny_tetmesh.png", lambda ax: render_tetmesh(ax, mesh))):
        f = plt.figure(figsize=(6, 6))
        ax = f.add_subplot(111, projection="3d")
        ax.view_init(elev=18, azim=130)
        fn(ax)
        f.savefig(HERE / name, dpi=130, bbox_inches="tight")
        print("Wrote", HERE / name)


if __name__ == "__main__":
    main()
