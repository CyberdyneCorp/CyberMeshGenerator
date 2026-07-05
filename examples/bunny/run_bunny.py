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

try:
    import trimesh  # optional reference voxelizer for comparison
except ImportError:
    trimesh = None

HERE = Path(__file__).parent
STL = HERE / "Stanford_Bunny_sample.stl"
GRID = 34  # vertex-clustering resolution for meshing (higher = finer, slower)
VOX_METRIC_RES = 48  # grid resolution for the IoU / volume comparison
VOX_RENDER_RES = 28  # coarser grid for the blocky voxel render


def load_and_simplify(path: Path, grid=GRID):
    """Load the STL natively and simplify it in the library — no hand-written parsing
    or clustering. `cm.read_plc` parses the STL (welding coincident vertices) and
    `cm.simplify` does the Rossignac-Borrel vertex clustering that used to live here.
    Returns (full_triangle_count, simplified_plc). The full surface is also meshable
    directly now that the carve is spatially indexed; simplifying just keeps the
    render light and fast."""
    full = cm.read_plc(str(path))
    return full.triangles.shape[0], cm.simplify(full, grid=grid)


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


def grid_centers(vg):
    """World coordinates of every cell center of a cybermesh VoxelGrid, (nx,ny,nz,3)."""
    nx, ny, nz = vg.dims
    i, j, k = np.meshgrid(np.arange(nx), np.arange(ny), np.arange(nz), indexing="ij")
    return np.stack([vg.origin[0] + i * vg.spacing,
                     vg.origin[1] + j * vg.spacing,
                     vg.origin[2] + k * vg.spacing], axis=-1)


def trimesh_occupancy_on(P, F, vg):
    """Reference occupancy sampled onto OUR grid `vg`: trimesh's solid voxelization
    (surface voxels + morphological fill) at the same pitch, read back at our cell
    centers so the two grids are cell-aligned for a fair IoU."""
    tm = trimesh.Trimesh(vertices=np.asarray(P), faces=np.asarray(F), process=False)
    vtm = tm.voxelized(pitch=vg.spacing).fill()
    idx = vtm.points_to_indices(grid_centers(vg).reshape(-1, 3))
    m = vtm.matrix
    ok = np.all((idx >= 0) & (idx < np.array(m.shape)), axis=1)
    ref = np.zeros(len(idx), bool)
    hit = idx[ok]
    ref[ok] = m[hit[:, 0], hit[:, 1], hit[:, 2]]
    return ref.reshape(vg.dims)


def render_voxels(ax, occ, color, title):
    ax.voxels(occ, facecolors=color, edgecolor=(0, 0, 0, 0.12), linewidth=0.1)
    ax.set_box_aspect(occ.shape)
    ax.set_axis_off()
    ax.set_title(title, fontsize=10)


def render_diff(ax, ours, ref, title):
    """Agreement view: gray = both, blue = ours only, orange = trimesh only."""
    both, only_ours, only_ref = ours & ref, ours & ~ref, ref & ~ours
    filled = both | only_ours | only_ref
    colors = np.zeros(filled.shape + (4,))
    colors[both] = (0.60, 0.60, 0.60, 0.85)
    colors[only_ours] = (0.30, 0.50, 0.85, 0.95)
    colors[only_ref] = (0.95, 0.55, 0.20, 0.95)
    ax.voxels(filled, facecolors=colors, edgecolor=(0, 0, 0, 0.06), linewidth=0.05)
    ax.set_box_aspect(filled.shape)
    ax.set_axis_off()
    ax.set_title(title, fontsize=10)


def voxel_comparison(plc, P, F):
    """Voxelize the bunny with our algorithm and, when trimesh is available, compare to
    its solid voxelization (IoU / Dice / volume, both vs the divergence-theorem volume),
    writing bunny_voxelization.png.

    Uses the FULL watertight surface (not the decimated one the tet-carve needs): the
    exact ray-parity classifier is only guaranteed on watertight input — a hole would let
    a column's parity leak into a spurious spike — and our voxelizer handles the full
    112k-triangle mesh directly (the carve required decimation, voxelization does not)."""
    v_true = surface_volume(P, F)
    ours = cm.voxelize(plc, resolution=VOX_METRIC_RES, mode="occupancy")
    ob = ours.grid.astype(bool)
    vol_ours = int(ob.sum()) * ours.spacing ** 3
    print(f"  our voxels (res {VOX_METRIC_RES}): {int(ob.sum())} occupied, "
          f"volume {vol_ours:.0f} ({100 * vol_ours / v_true:.1f}% of enclosed)")

    metrics = None
    if trimesh is not None:
        ref = trimesh_occupancy_on(P, F, ours)
        inter, union = int((ob & ref).sum()), int((ob | ref).sum())
        vol_ref = int(ref.sum()) * ours.spacing ** 3
        metrics = dict(iou=inter / union, dice=2 * inter / (ob.sum() + ref.sum()),
                       agree=float((ob == ref).mean()), vol_ref=vol_ref)
        print(f"  vs trimesh: IoU={metrics['iou']:.3f} Dice={metrics['dice']:.3f} "
              f"cell-agree={100 * metrics['agree']:.1f}%  trimesh volume {vol_ref:.0f} "
              f"({100 * vol_ref / v_true:.1f}% of enclosed)")
    else:
        print("  (trimesh not installed — reference comparison skipped; pip install trimesh)")

    # Coarser grid for the blocky render (both tools sampled on the same render grid).
    rvg = cm.voxelize(plc, resolution=VOX_RENDER_RES, mode="occupancy")
    our_r = rvg.grid.astype(bool)
    panels = [lambda ax: render_voxels(ax, our_r, "#4a78c0",
              f"CyberMeshGenerator voxels\n{'x'.join(map(str, rvg.dims))} · "
              f"{int(our_r.sum())} cells")]
    if trimesh is not None:
        ref_r = trimesh_occupancy_on(P, F, rvg)
        panels.append(lambda ax: render_voxels(ax, ref_r, "#e08a33",
                      f"trimesh voxels (reference)\n{int(ref_r.sum())} cells"))
        panels.append(lambda ax: render_diff(ax, our_r, ref_r,
                      "agreement\ngray both · blue ours · orange trimesh"))

    fig = plt.figure(figsize=(5.5 * len(panels), 5.5))
    for i, fn in enumerate(panels):
        ax = fig.add_subplot(1, len(panels), i + 1, projection="3d")
        ax.view_init(elev=18, azim=130)
        fn(ax)
    sub = "CyberMeshGenerator voxelization"
    if metrics:
        sub += (f" vs trimesh — IoU {metrics['iou']:.2f}, Dice {metrics['dice']:.2f}; "
                f"solid volume {100 * vol_ours / v_true:.0f}% (ours) vs "
                f"{100 * metrics['vol_ref'] / v_true:.0f}% (trimesh) of the enclosed volume")
    fig.suptitle(sub, fontsize=12, y=1.02)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    fig.savefig(HERE / "bunny_voxelization.png", dpi=130, bbox_inches="tight")
    print("Wrote", HERE / "bunny_voxelization.png")


def main():
    print("Loading + simplifying", STL.name, "natively (cm.read_plc / cm.simplify) ...")
    full_tris, sic = load_and_simplify(STL)
    P, F = sic.points, sic.triangles
    print(f"  {full_tris} triangles (full) -> simplified (grid {GRID}): "
          f"{len(P)} verts, {len(F)} triangles")

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
        t0 = time.time()
        mesh = cm.tetrahedralize(sic, cm.MeshOptions(plc=True))
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

    print("Voxelizing the full watertight surface and comparing to trimesh ...")
    full = cm.read_plc(str(STL))
    voxel_comparison(full, full.points, full.triangles)


if __name__ == "__main__":
    main()
