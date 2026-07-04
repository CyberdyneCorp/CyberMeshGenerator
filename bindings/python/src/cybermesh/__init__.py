"""CyberMesh — Python bindings for CyberMeshGenerator.

Thin, NumPy-interoperable layer over the C ABI shim (``bindings/c``). Point,
tetrahedron, and face arrays cross the boundary as NumPy arrays without copying.

The packaged wheel builds a nanobind extension (``cybermesh._core``); this module
also provides a dependency-light ctypes fallback over the shared C ABI library so
the binding contract is usable and testable without the compiled extension. Both
expose the same surface — all meshing logic lives in the C++ core.

    import numpy as np, cybermesh as cm
    pts = np.array([[0,0,0],[1,0,0],[0,1,0],[0,0,1]], dtype=np.float64)
    mesh = cm.delaunay(pts)
    mesh.points   # -> (4, 3) float64 ndarray
    mesh.tets     # -> (1, 4) int32 ndarray
"""
from __future__ import annotations

import ctypes
import ctypes.util
from dataclasses import dataclass

import numpy as np

try:  # Prefer the compiled nanobind extension when the wheel provides it.
    from ._core import delaunay as delaunay, tetrahedralize as tetrahedralize  # type: ignore
    _BACKEND = "nanobind"
except ImportError:  # ctypes fallback over the C ABI shared library.
    _BACKEND = "ctypes"

    def _load_lib() -> ctypes.CDLL:
        name = ctypes.util.find_library("cmg_c") or "libcmg_c.so"
        return ctypes.CDLL(name)

    _lib = _load_lib()
    _lib.cmg_options_create.restype = ctypes.c_void_p
    _lib.cmg_delaunay.restype = ctypes.c_int
    _lib.cmg_delaunay.argtypes = [
        ctypes.POINTER(ctypes.c_double), ctypes.c_size_t, ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_void_p), ctypes.c_char_p, ctypes.c_size_t]
    _lib.cmg_mesh_num_points.restype = ctypes.c_size_t
    _lib.cmg_mesh_num_tets.restype = ctypes.c_size_t
    _lib.cmg_mesh_points.restype = ctypes.POINTER(ctypes.c_double)
    _lib.cmg_mesh_tets.restype = ctypes.POINTER(ctypes.c_int)

    @dataclass
    class Mesh:
        points: np.ndarray
        tets: np.ndarray

    def delaunay(points: np.ndarray) -> Mesh:
        pts = np.ascontiguousarray(points, dtype=np.float64)
        if pts.ndim != 2 or pts.shape[1] != 3:
            raise ValueError("points must be an (N, 3) array")
        opts = _lib.cmg_options_create()
        out = ctypes.c_void_p()
        err = ctypes.create_string_buffer(256)
        st = _lib.cmg_delaunay(
            pts.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
            pts.shape[0], opts, ctypes.byref(out), err, 256)
        if st != 0:
            raise RuntimeError(err.value.decode() or f"cmg error {st}")
        npts = _lib.cmg_mesh_num_points(out)
        ntet = _lib.cmg_mesh_num_tets(out)
        p = np.ctypeslib.as_array(_lib.cmg_mesh_points(out), (npts, 3)).copy()
        t = np.ctypeslib.as_array(_lib.cmg_mesh_tets(out), (ntet, 4)).copy()
        return Mesh(points=p, tets=t)

__all__ = ["delaunay", "tetrahedralize"]
