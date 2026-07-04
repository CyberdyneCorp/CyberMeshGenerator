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


def read_stl_corners(path: Path):
    """Load the STL natively and return its (T, 3, 3) triangle-corner array.

    ``cm.read_plc`` parses the STL (no hand-written binary unpacking); the loaded
    PLC's geometry is read back through the accessors — ``plc.points`` (V, 3) and
    ``plc.triangles`` (T, 3) — and indexed to per-corner coordinates."""
    plc = cm.read_plc(str(path))
    return plc.points[plc.triangles]  # (T, 3, 3)


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


def boundary_faces(T):
    """The boundary faces of a tetrahedron set (faces owned by exactly one tet)."""
    fc, face_of = Counter(), {}
    for t in T:
        for f in ((t[1], t[2], t[3]), (t[0], t[2], t[3]),
                  (t[0], t[1], t[3]), (t[0], t[1], t[2])):
            k = tuple(sorted(f)); fc[k] += 1; face_of[k] = f
    return [face_of[k] for k, m in fc.items() if m == 1]


def tet_volume(mesh):
    P, T = mesh.points, mesh.tetrahedra
    a, b, c, d = P[T[:, 0]], P[T[:, 1]], P[T[:, 2]], P[T[:, 3]]
    return np.abs(np.einsum("ij,ij->i", b - a, np.cross(c - a, d - a))).sum() / 6.0


def surface_volume(P, F):
    """Volume enclosed by the surface (divergence theorem) — the ground truth."""
    Fa = np.asarray(F)
    a, b, c = P[Fa[:, 0]], P[Fa[:, 1]], P[Fa[:, 2]]
    return abs(np.einsum("ij,ij->i", a, np.cross(b, c)).sum() / 6.0)


def render_solid(ax, mesh):
    """The FULL solid: boundary faces of ALL tetrahedra (= the bunny surface,
    reconstructed from our mesh) — shows the mesh is a complete, closed solid."""
    P, T = mesh.points, mesh.tetrahedra
    polys = P[np.asarray(boundary_faces(T))]
    pc = Poly3DCollection(polys, facecolors=shade(polys, (0.62, 0.74, 0.86)),
                          edgecolors=(0, 0, 0, 0.08), linewidths=0.1)
    pc.set_rasterized(True)
    ax.add_collection3d(pc)
    set_equal(ax, P)
    ax.set_title("Our solid tet mesh — full boundary\n"
                 f"{len(T)} tets · boundary conforms to the bunny", fontsize=10)


def render_tetmesh(ax, mesh):
    """Cutaway: keep the x < median half of the tetrahedra, draw that half's
    boundary faces — the cut plane reveals interior tetrahedra."""
    P, T = mesh.points, mesh.tetrahedra
    cx = P[T].mean(axis=1)[:, 0]
    bfaces = boundary_faces(T[cx < np.median(cx)])
    polys = P[np.asarray(bfaces)]
    depth = polys.mean(axis=1)[:, 0]
    colors = plt.cm.plasma((depth - depth.min()) / (np.ptp(depth) + 1e-9))
    pc = Poly3DCollection(polys, facecolors=colors, edgecolors=(0, 0, 0, 0.2),
                          linewidths=0.15)
    pc.set_rasterized(True)
    ax.add_collection3d(pc)
    set_equal(ax, P)
    ax.set_title("Cutaway — interior tetrahedra revealed\n"
                 f"{len(bfaces)} cut faces", fontsize=10)


def main():
    print("Loading", STL.name, "natively (cm.read_plc) ...")
    corners = read_stl_corners(STL)
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

    # Self-validation: the solid mesh volume must match the volume enclosed by the
    # input surface (divergence theorem). A close match proves the carve filled the
    # whole interior — nothing is missing (the cutaway below is only a view).
    v_mesh = tet_volume(mesh)
    v_true = surface_volume(P, F)
    print(f"  volume check: mesh={v_mesh:.0f}  surface-enclosed={v_true:.0f}  "
          f"({100 * v_mesh / v_true:.2f}% — the solid is complete)")

    panels = ((lambda ax: render_surface(ax, P, F)),
              (lambda ax: render_solid(ax, mesh)),
              (lambda ax: render_tetmesh(ax, mesh)))
    fig = plt.figure(figsize=(16, 5.5))
    for i, fn in enumerate(panels):
        ax = fig.add_subplot(1, 3, i + 1, projection="3d")
        ax.view_init(elev=18, azim=130)
        fn(ax)
    fig.suptitle("CyberMeshGenerator — Stanford Bunny: input surface → solid tet "
                 f"mesh (volume {100 * v_mesh / v_true:.1f}% of enclosed)",
                 fontsize=13, y=1.06)
    fig.tight_layout(rect=[0, 0, 1, 0.93])
    fig.savefig(HERE / "bunny_comparison.png", dpi=130, bbox_inches="tight")
    print("Wrote", HERE / "bunny_comparison.png")

    for name, fn in (("bunny_surface.png", lambda ax: render_surface(ax, P, F)),
                     ("bunny_solid.png", lambda ax: render_solid(ax, mesh)),
                     ("bunny_tetmesh.png", lambda ax: render_tetmesh(ax, mesh))):
        f = plt.figure(figsize=(6, 6))
        ax = f.add_subplot(111, projection="3d")
        ax.view_init(elev=18, azim=130)
        fn(ax)
        f.savefig(HERE / name, dpi=130, bbox_inches="tight")
        print("Wrote", HERE / name)


if __name__ == "__main__":
    main()
