"""Verify cybermesh.voxelize: an occupancy grid whose occupied volume tracks the
solid, and a signed-distance field negative inside / positive outside. Run with
CMG_C_LIB pointed at libcmg_c.so."""
import numpy as np

import cybermesh as cm


def unit_cube(side=1.0):
    """A watertight axis-aligned cube [0, side]^3 as a triangulated PLC."""
    p = cm.PLC()
    s = side
    corners = [[0, 0, 0], [s, 0, 0], [s, s, 0], [0, s, 0],
               [0, 0, s], [s, 0, s], [s, s, s], [0, s, s]]
    p.add_points(corners)
    # Six faces, each split into two triangles (outward winding not required for
    # the exact inside/outside classifier, but kept consistent here).
    faces = [
        (0, 3, 2, 1),  # z = 0
        (4, 5, 6, 7),  # z = s
        (0, 1, 5, 4),  # y = 0
        (2, 3, 7, 6),  # y = s
        (1, 2, 6, 5),  # x = s
        (0, 4, 7, 3),  # x = 0
    ]
    for a, b, c, d in faces:
        p.add_facet([a, b, c])
        p.add_facet([a, c, d])
    return p


def brick(X, Y, Z):
    """A watertight axis-aligned box [0,X]x[0,Y]x[0,Z] as a triangulated PLC."""
    p = cm.PLC()
    p.add_points([[0, 0, 0], [X, 0, 0], [X, Y, 0], [0, Y, 0],
                  [0, 0, Z], [X, 0, Z], [X, Y, Z], [0, Y, Z]])
    for a, b, c, d in [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
                       (2, 3, 7, 6), (1, 2, 6, 5), (0, 4, 7, 3)]:
        p.add_facet([a, b, c])
        p.add_facet([a, c, d])
    return p


def check_grid_axes():
    """Regression: grid[i,j,k] must map to world cell (x_i, y_j, z_k) on a NON-cubic
    grid. The C ABI lays cells out flat as (k*ny+j)*nx+i, so a naive reshape((nx,ny,nz))
    silently transposes the axes when nx != ny != nz — this caught exactly that."""
    for X, Y, Z in [(2.0, 1.0, 1.0), (1.0, 2.0, 1.0), (1.0, 1.0, 2.0), (3.0, 1.0, 2.0)]:
        vg = cm.voxelize(brick(X, Y, Z), resolution=20, pad=1)
        nx, ny, nz = vg.dims
        # the axis with the largest extent must get the most cells
        assert vg.dims[np.argmax([X, Y, Z])] == max(vg.dims), (X, Y, Z, vg.dims)
        i, j, k = np.meshgrid(np.arange(nx), np.arange(ny), np.arange(nz), indexing="ij")
        cx = vg.origin[0] + i * vg.spacing
        cy = vg.origin[1] + j * vg.spacing
        cz = vg.origin[2] + k * vg.spacing
        expect = ((cx > 0) & (cx < X) & (cy > 0) & (cy < Y) &
                  (cz > 0) & (cz < Z)).astype(np.uint8)
        agree = float((expect == vg.grid).mean())
        assert agree > 0.98, (X, Y, Z, vg.dims, agree)


def main():
    check_grid_axes()

    side = 2.0
    cube = unit_cube(side)
    solid_volume = side ** 3

    # --- occupancy -------------------------------------------------------
    vg = cm.voxelize(cube, resolution=32, mode="occupancy", pad=1)
    assert vg.grid.dtype == np.uint8, vg.grid.dtype
    assert vg.grid.ndim == 3 and vg.grid.shape == vg.dims, vg.grid.shape
    assert vg.origin.shape == (3,) and vg.origin.dtype == np.float64
    assert vg.spacing > 0.0

    occupied = int(vg.grid.sum())
    total = int(vg.grid.size)
    assert 0 < occupied < total, (occupied, total)
    occ_volume = occupied * (vg.spacing ** 3)
    rel = abs(occ_volume - solid_volume) / solid_volume
    assert rel < 0.15, (occ_volume, solid_volume, rel)

    # integer mode selector is accepted too, and is deterministic
    vg0 = cm.voxelize(cube, resolution=32, mode=0, pad=1)
    assert np.array_equal(vg0.grid, vg.grid)

    # --- signed distance -------------------------------------------------
    sdf = cm.voxelize(cube, resolution=32, mode="sdf", pad=1)
    assert sdf.grid.dtype == np.float32, sdf.grid.dtype
    assert sdf.grid.shape == vg.grid.shape
    assert (sdf.grid < 0).any(), "no interior (negative) cells"
    assert (sdf.grid > 0).any(), "no exterior (positive) cells"

    # sign agrees with occupancy: occupied cells are inside (<= 0)
    inside_frac = float((sdf.grid[vg.grid.astype(bool)] <= sdf.spacing).mean())
    assert inside_frac > 0.9, inside_frac

    print(f"PASS: voxelize occupancy {vg.dims} occ={occupied}/{total} "
          f"vol {occ_volume:.3f}~{solid_volume:.3f} (rel {rel:.3f}); "
          f"sdf neg={(sdf.grid < 0).sum()} pos={(sdf.grid > 0).sum()}")


if __name__ == "__main__":
    main()
