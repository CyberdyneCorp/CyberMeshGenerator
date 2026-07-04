#!/usr/bin/env python3
"""Eiffel Tower example — Delaunay tetrahedralization head-to-head vs TetGen.

The Eiffel model is an open lattice (a truss, not a watertight solid), so — like the
antenna — there is no closed interior to carve. This example instead does the thing
that IS well-defined and directly comparable: the Delaunay tetrahedralization of the
model's vertices, computed with **CyberMeshGenerator** and, if a `tetgen` binary is
available, with **real TetGen** — then compares them (tet count, volume, and the
fraction of identical tetrahedra).

Run (headless; writes PNGs):

    EIFFEL_STL=/path/to/Eiffel_tower_sample.STL \
    TETGEN_BIN=/path/to/tetgen \
    CMG_C_LIB=$(readlink -f "$(find ../../build-py -name libcmg_c.so | head -1)") \
    PYTHONPATH=../../bindings/python/src python3 run_eiffel.py

The 35 MB STL is not vendored — point `EIFFEL_STL` at your copy (default: this
directory or ~/Downloads). `TETGEN_BIN` is optional; without it the TetGen panel is
skipped. Requires numpy + matplotlib.
"""
from __future__ import annotations

import os
import shutil
import subprocess
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
GRID = 48


def find_stl() -> Path:
    for c in (os.environ.get("EIFFEL_STL"),
              HERE / "Eiffel_tower_sample.STL",
              Path.home() / "Downloads" / "Eiffel_tower_sample.STL"):
        if c and Path(c).exists():
            return Path(c)
    raise SystemExit("Eiffel STL not found; set EIFFEL_STL=/path/to/Eiffel_tower_sample.STL")


def find_tetgen():
    for c in (os.environ.get("TETGEN_BIN"), shutil.which("tetgen"),
              "/tmp/tetgen-build/tetgen"):
        if c and Path(c).exists():
            return c
    return None


def read_stl_corners(path: Path):
    """Load the STL natively (cm.read_plc — ASCII or binary) and read its geometry
    back through the accessors as a (T, 3, 3) triangle-corner array."""
    plc = cm.read_plc(str(path))
    return plc.points[plc.triangles]


def cluster_decimate(corners, grid=GRID):
    V = corners.reshape(-1, 3)
    lo, ext = V.min(0), (V.max(0) - V.min(0)).max()
    cell = np.floor((V - lo) / ext * grid).astype(int)
    key = cell[:, 0] * grid * grid * 4 + cell[:, 1] * grid * 2 + cell[:, 2]
    uk, inv = np.unique(key, return_inverse=True)
    reps = np.zeros((len(uk), 3)); cnt = np.zeros(len(uk))
    np.add.at(reps, inv, V); np.add.at(cnt, inv, 1); reps /= cnt[:, None]
    vi = inv.reshape(len(corners), 3)
    faces, seen = [], set()
    for a, b, c in vi:
        if a == b or b == c or a == c:
            continue
        k = tuple(sorted((int(a), int(b), int(c))))
        if k not in seen:
            seen.add(k); faces.append((int(a), int(b), int(c)))
    return reps, faces


def tet_volume(P, T):
    a, b, c, d = P[T[:, 0]], P[T[:, 1]], P[T[:, 2]], P[T[:, 3]]
    return np.abs(np.einsum("ij,ij->i", b - a, np.cross(c - a, d - a))).sum() / 6.0


def run_tetgen(binary, P):
    node = Path("/tmp/eiffel_cmp.node")
    with open(node, "w") as fo:
        fo.write(f"{len(P)} 3 0 0\n")
        for i, p in enumerate(P):
            fo.write(f"{i} {p[0]} {p[1]} {p[2]}\n")
    for x in ("/tmp/eiffel_cmp.1.ele", "/tmp/eiffel_cmp.1.node"):
        if os.path.exists(x):
            os.remove(x)
    t = time.time()
    subprocess.run([binary, str(node)], capture_output=True)
    dt = time.time() - t
    ele = Path("/tmp/eiffel_cmp.1.ele")
    if not ele.exists():
        return None
    # Load TetGen's output through the native mesh reader (.ele + companion .node) —
    # no hand-parsing of the TetGen index format.
    tg = cm.read_mesh(str(ele))
    return tg.points, tg.tetrahedra, dt


def set_equal(ax, V):
    lo, hi = V.min(0), V.max(0)
    ax.set_xlim(lo[0], hi[0]); ax.set_ylim(lo[1], hi[1]); ax.set_zlim(lo[2], hi[2])
    ax.set_box_aspect(hi - lo); ax.set_axis_off()


def shade(polys, base):
    n = np.cross(polys[:, 1] - polys[:, 0], polys[:, 2] - polys[:, 0])
    n /= (np.linalg.norm(n, axis=1, keepdims=True) + 1e-12)
    lam = 0.30 + 0.70 * np.abs(n @ np.array([0.5, 0.4, 0.75]))
    return np.clip(np.asarray(base)[None, :] * lam[:, None], 0, 1)


def render_surface(ax, P, F, title):
    polys = P[np.asarray(F)]
    pc = Poly3DCollection(polys, facecolors=shade(polys, (0.55, 0.45, 0.35)),
                          edgecolors="none")
    pc.set_rasterized(True); ax.add_collection3d(pc); set_equal(ax, P)
    ax.set_title(title, fontsize=10)


def render_dt(ax, P, T, title, cmap):
    cz = P[T].mean(axis=1)[:, 2]
    kept = T[cz < np.median(cz)]
    fc, face_of = Counter(), {}
    for t in kept:
        for f in ((t[1], t[2], t[3]), (t[0], t[2], t[3]),
                  (t[0], t[1], t[3]), (t[0], t[1], t[2])):
            k = tuple(sorted(f)); fc[k] += 1; face_of[k] = f
    bf = P[np.asarray([face_of[k] for k, m in fc.items() if m == 1])]
    d = bf.mean(axis=1)[:, 2]
    pc = Poly3DCollection(bf, facecolors=cmap((d - d.min()) / (np.ptp(d) + 1e-9)),
                          edgecolors=(0, 0, 0, 0.15), linewidths=0.1)
    pc.set_rasterized(True); ax.add_collection3d(pc); set_equal(ax, P)
    ax.set_title(title, fontsize=10)


def main():
    stl = find_stl()
    print("Loading", stl.name, "natively (cm.read_plc) ...")
    corners = read_stl_corners(stl)
    print(f"  {len(corners)} triangles (full)")
    P, F = cluster_decimate(corners)
    print(f"  decimated (grid {GRID}): {len(P)} verts, {len(F)} triangles")

    t = time.time(); mesh = cm.delaunay(P); dt_ours = time.time() - t
    v_ours = tet_volume(mesh.points, mesh.tetrahedra)
    print(f"OURS   Delaunay: {mesh.tetrahedra.shape[0]} tets, vol={v_ours:.0f}, "
          f"{dt_ours * 1000:.0f} ms")

    tg = find_tetgen()
    tg_result = run_tetgen(tg, P) if tg else None
    panels = [lambda ax: render_surface(ax, P, F,
              f"Eiffel surface (open lattice)\n{len(corners)} tris → decimated {len(F)}")]
    panels.append(lambda ax: render_dt(ax, mesh.points, mesh.tetrahedra,
                  f"CyberMeshGenerator Delaunay\n{mesh.tetrahedra.shape[0]} tets · "
                  f"vol {v_ours:.0f}", plt.cm.viridis))
    if tg_result:
        TP, TT, dt_tg = tg_result
        v_tg = tet_volume(TP, TT)
        ours = {tuple(sorted(t)) for t in mesh.tetrahedra}
        same = len({tuple(sorted(t)) for t in TT} & ours)
        print(f"TETGEN Delaunay: {TT.shape[0]} tets, vol={v_tg:.0f}, {dt_tg * 1000:.0f} ms")
        print(f"COMPARE        : identical tetrahedra {same}/{len(ours)} = "
              f"{100 * same / len(ours):.2f}%  | Δvolume = {abs(v_ours - v_tg):.3g}")
        panels.append(lambda ax: render_dt(ax, TP, TT,
                      f"real TetGen Delaunay\n{TT.shape[0]} tets · vol {v_tg:.0f}",
                      plt.cm.viridis))
        sub = (f"CyberMeshGenerator vs TetGen — Delaunay of the same {len(P)} points: "
               f"{100 * same / len(ours):.1f}% identical tetrahedra")
    else:
        print("TetGen not found (set TETGEN_BIN); showing CyberMeshGenerator only.")
        sub = "CyberMeshGenerator Delaunay (set TETGEN_BIN for the TetGen comparison)"

    fig = plt.figure(figsize=(6 * len(panels), 6))
    for i, fn in enumerate(panels):
        ax = fig.add_subplot(1, len(panels), i + 1, projection="3d")
        ax.view_init(elev=14, azim=-60); fn(ax)
    fig.suptitle(sub, fontsize=13, y=1.02)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "eiffel_comparison.png", dpi=130, bbox_inches="tight")
    print("Wrote", HERE / "eiffel_comparison.png")


if __name__ == "__main__":
    main()
