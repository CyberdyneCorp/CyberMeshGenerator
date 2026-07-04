#!/usr/bin/env python3
"""Antenna example — load a textured OBJ surface, tetrahedralize it with
CyberMeshGenerator, and render the original textured mesh next to the volumetric
tetrahedral mesh our library produced.

Run (headless-friendly, writes PNGs):

    # build the shared C ABI once (from the repo root):
    #   cmake -S . -B build-py -DCMG_BUILD_C_ABI=ON -DCMG_BUILD_SHARED=ON \
    #         -DCMG_BUILD_TESTS=OFF -DCMG_BUILD_CLI=OFF && cmake --build build-py -j
    CMG_C_LIB=$(find ../../build-py -name libcmg_c.so | head -1) \
    PYTHONPATH=../../bindings/python/src python3 run_antenna.py

Only requires: numpy, matplotlib, Pillow (PIL) — plus the cybermesh binding.
"""
from __future__ import annotations

import os
import time
from pathlib import Path

import matplotlib
matplotlib.use("Agg")  # headless / offscreen
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from PIL import Image

import cybermesh as cm

HERE = Path(__file__).parent
OBJ = HERE / "Antenna.obj"
TEX = HERE / "Antenna.jpg"


def parse_obj(path: Path):
    """Return vertices (V,3), texcoords (T,2), and triangles as
    (vertex_index[3], texcoord_index[3]) — polygons are fan-triangulated."""
    verts, texs, tris = [], [], []
    for line in path.read_text().splitlines():
        if line.startswith("v "):
            p = line.split()
            verts.append((float(p[1]), float(p[2]), float(p[3])))
        elif line.startswith("vt "):
            p = line.split()
            texs.append((float(p[1]), float(p[2])))
        elif line.startswith("f "):
            toks = line.split()[1:]
            vs, ts = [], []
            for tok in toks:
                a = tok.split("/")
                vs.append(int(a[0]) - 1)
                ts.append(int(a[1]) - 1 if len(a) > 1 and a[1] else 0)
            for i in range(1, len(vs) - 1):  # fan-triangulate
                tris.append(((vs[0], vs[i], vs[i + 1]),
                             (ts[0], ts[i], ts[i + 1])))
    return (np.asarray(verts, float), np.asarray(texs, float), tris)


def face_texture_colors(texs, tris, tex_path: Path):
    """Sample the JPG texture at each triangle's average UV -> per-face RGB."""
    img = np.asarray(Image.open(tex_path).convert("RGB"))
    h, w = img.shape[:2]
    cols = np.empty((len(tris), 3), float)
    for k, (_, ti) in enumerate(tris):
        uv = texs[list(ti)].mean(axis=0)
        px = min(w - 1, max(0, int(uv[0] % 1.0 * (w - 1))))
        py = min(h - 1, max(0, int((1.0 - uv[1] % 1.0) * (h - 1))))
        cols[k] = img[py, px] / 255.0
    return cols


def set_equal(ax, V):
    lo, hi = V.min(axis=0), V.max(axis=0)
    ax.set_xlim(lo[0], hi[0])
    ax.set_ylim(lo[1], hi[1])
    ax.set_zlim(lo[2], hi[2])
    ax.set_box_aspect(hi - lo)  # true geometric proportions (a tall thin tower)
    ax.set_axis_off()


def shade(polys, colors, light=(0.4, 0.5, 0.75)):
    """Lambert-shade per-face colors by the angle to a light direction."""
    a, b, c = polys[:, 0], polys[:, 1], polys[:, 2]
    n = np.cross(b - a, c - a)
    n /= (np.linalg.norm(n, axis=1, keepdims=True) + 1e-12)
    ld = np.asarray(light) / np.linalg.norm(light)
    lam = 0.35 + 0.65 * np.abs(n @ ld)[:, None]
    out = colors.copy()
    out[:, :3] = np.clip(colors[:, :3] * lam, 0, 1)
    return out


def render_original(ax, V, tris, colors):
    polys = V[[list(vi) for vi, _ in tris]]
    rgba = np.concatenate([colors, np.ones((len(colors), 1))], axis=1)
    pc = Poly3DCollection(polys, facecolors=shade(polys, rgba), edgecolors="none")
    pc.set_rasterized(True)
    ax.add_collection3d(pc)
    set_equal(ax, V)
    ax.set_title("Original textured surface\n"
                 f"{len(V)} vertices · {len(tris)} triangles", fontsize=10)


def render_tetmesh(ax, mesh):
    """Cutaway of the volumetric mesh: keep the lower-z half of the tetrahedra and
    draw the boundary faces of that half — the cut plane reveals interior tets."""
    P, T = mesh.points, mesh.tetrahedra
    cz = P[T].mean(axis=1)[:, 2]
    keep = cz < np.median(cz)
    kept = T[keep]
    # boundary faces of the kept subset = faces appearing exactly once
    from collections import Counter
    fc = Counter()
    face_of = {}
    for t in kept:
        for f in ((t[1], t[2], t[3]), (t[0], t[2], t[3]),
                  (t[0], t[1], t[3]), (t[0], t[1], t[2])):
            key = tuple(sorted(f))
            fc[key] += 1
            face_of[key] = f
    bfaces = [face_of[k] for k, n in fc.items() if n == 1]
    polys = P[[list(f) for f in bfaces]]
    depth = P[[list(f) for f in bfaces]].mean(axis=1)[:, 2]
    norm = (depth - depth.min()) / (np.ptp(depth) + 1e-9)
    colors = plt.cm.viridis(norm)
    ls = LightSource(azdeg=225, altdeg=45)
    pc = Poly3DCollection(polys, facecolors=colors, edgecolors=(0, 0, 0, 0.15),
                          linewidths=0.15)
    pc.set_rasterized(True)
    ax.add_collection3d(pc)
    set_equal(ax, P)
    ax.set_title("CyberMeshGenerator tetrahedral mesh (cutaway)\n"
                 f"{len(T)} tetrahedra · {len(bfaces)} cut faces shown", fontsize=10)


def main():
    print("Parsing", OBJ.name, "...")
    V, VT, tris = parse_obj(OBJ)
    colors = face_texture_colors(VT, tris, TEX)
    print(f"  {len(V)} vertices, {len(tris)} triangles, texture {TEX.name}")

    cache = HERE / "_tetmesh_cache.npz"
    if cache.exists() and os.environ.get("CMG_NO_CACHE") is None:
        d = np.load(cache)
        mesh = cm.Mesh(points=d["points"], tetrahedra=d["tets"],
                       faces=np.empty((0, 3), np.int32),
                       tet_markers=np.empty((0,), np.int32),
                       face_markers=np.empty((0,), np.int32))
        print(f"  loaded cached tet mesh: {mesh.tetrahedra.shape[0]} tetrahedra")
    else:
        print("Tetrahedralizing with CyberMeshGenerator (Delaunay of the vertices)...")
        t0 = time.time()
        mesh = cm.delaunay(V)
        print(f"  {mesh.tetrahedra.shape[0]} tetrahedra in {time.time() - t0:.1f}s")
        np.savez_compressed(cache, points=mesh.points, tets=mesh.tetrahedra)

    fig = plt.figure(figsize=(14, 5))
    for i, (title, fn) in enumerate((
            ("original", lambda ax: render_original(ax, V, tris, colors)),
            ("tetmesh", lambda ax: render_tetmesh(ax, mesh)))):
        ax = fig.add_subplot(1, 2, i + 1, projection="3d")
        ax.view_init(elev=12, azim=-75)
        fn(ax)
    fig.suptitle("CyberMeshGenerator — Antenna.obj", fontsize=13)
    fig.tight_layout()
    out = HERE / "antenna_comparison.png"
    fig.savefig(out, dpi=130, bbox_inches="tight")
    print("Wrote", out)

    # also individual panels
    for name, fn in (("antenna_original.png",
                      lambda ax: render_original(ax, V, tris, colors)),
                     ("antenna_tetmesh.png",
                      lambda ax: render_tetmesh(ax, mesh))):
        f = plt.figure(figsize=(7, 6))
        ax = f.add_subplot(111, projection="3d")
        ax.view_init(elev=12, azim=-75)
        fn(ax)
        f.savefig(HERE / name, dpi=130, bbox_inches="tight")
        print("Wrote", HERE / name)


if __name__ == "__main__":
    main()
