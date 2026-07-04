"""CyberMesh — Python bindings for CyberMeshGenerator.

Thin, NumPy-interoperable layer over the C ABI shim (``bindings/c``). Point,
tetrahedron, and face arrays cross the boundary as NumPy arrays without copying.

The packaged wheel builds a nanobind extension (``cybermesh._core``); this module
also provides a dependency-light ctypes fallback over the shared C ABI library so
the binding contract is usable and testable without the compiled extension. Both
expose the same surface — all meshing logic lives in the C++ core.

    import numpy as np, cybermesh as cm

    # Point-set Delaunay:
    pts = np.array([[0,0,0],[1,0,0],[0,1,0],[0,0,1]], dtype=np.float64)
    mesh = cm.delaunay(pts)
    mesh.points   # -> (4, 3) float64 ndarray
    mesh.tets     # -> (1, 4) int32 ndarray

    # Boundary-conforming (PLC) meshing:
    plc = cm.PLC()
    plc.add_points(cube_corners)          # (8, 3)
    for tri in cube_triangles:            # 12 triangles closing the surface
        plc.add_facet(tri, marker=1)
    mesh = cm.tetrahedralize(plc, cm.MeshOptions(plc=True))
    mesh.tetrahedra  # -> (T, 4) int32 ndarray

The shared library is located via the ``CMG_C_LIB`` environment variable when
set, otherwise via ``ctypes.util.find_library``.
"""
from __future__ import annotations

import ctypes
import ctypes.util
import os
from dataclasses import dataclass
from typing import Optional, Sequence

import numpy as np

_c_double_p = ctypes.POINTER(ctypes.c_double)
_c_int_p = ctypes.POINTER(ctypes.c_int)
_c_void_pp = ctypes.POINTER(ctypes.c_void_p)


def _load_lib() -> ctypes.CDLL:
    """Locate and open the C ABI shared library.

    Honours ``CMG_C_LIB`` (an explicit path) first so tests can point at a
    freshly built tree; otherwise falls back to the system loader search.
    """
    env = os.environ.get("CMG_C_LIB")
    if env:
        return ctypes.CDLL(env)
    name = ctypes.util.find_library("cmg_c") or "libcmg_c.so"
    return ctypes.CDLL(name)


def _bind(lib: ctypes.CDLL) -> None:
    """Declare argument/return types for every C ABI symbol we call."""
    lib.cmg_plc_create.restype = ctypes.c_void_p
    lib.cmg_plc_destroy.argtypes = [ctypes.c_void_p]
    lib.cmg_options_create.restype = ctypes.c_void_p
    lib.cmg_options_destroy.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_destroy.argtypes = [ctypes.c_void_p]

    lib.cmg_plc_set_points.restype = ctypes.c_int
    lib.cmg_plc_set_points.argtypes = [ctypes.c_void_p, _c_double_p, ctypes.c_size_t]
    lib.cmg_plc_add_facet.restype = ctypes.c_int
    lib.cmg_plc_add_facet.argtypes = [
        ctypes.c_void_p, _c_int_p, ctypes.c_size_t, ctypes.c_int]
    lib.cmg_plc_add_hole.restype = ctypes.c_int
    lib.cmg_plc_add_hole.argtypes = [
        ctypes.c_void_p, ctypes.c_double, ctypes.c_double, ctypes.c_double]
    lib.cmg_plc_add_region.restype = ctypes.c_int
    lib.cmg_plc_add_region.argtypes = [
        ctypes.c_void_p, ctypes.c_double, ctypes.c_double, ctypes.c_double,
        ctypes.c_double, ctypes.c_double]

    lib.cmg_options_set_plc.restype = ctypes.c_int
    lib.cmg_options_set_plc.argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.cmg_options_set_max_volume.restype = ctypes.c_int
    lib.cmg_options_set_max_volume.argtypes = [ctypes.c_void_p, ctypes.c_double]
    lib.cmg_options_set_quality.restype = ctypes.c_int
    lib.cmg_options_set_quality.argtypes = [
        ctypes.c_void_p, ctypes.c_double, ctypes.c_double]
    lib.cmg_options_set_preserve_edges.restype = ctypes.c_int
    lib.cmg_options_set_preserve_edges.argtypes = [ctypes.c_void_p, ctypes.c_int]

    lib.cmg_tetrahedralize.restype = ctypes.c_int
    lib.cmg_tetrahedralize.argtypes = [
        ctypes.c_void_p, ctypes.c_void_p, _c_void_pp,
        ctypes.c_char_p, ctypes.c_size_t]
    lib.cmg_delaunay.restype = ctypes.c_int
    lib.cmg_delaunay.argtypes = [
        _c_double_p, ctypes.c_size_t, ctypes.c_void_p,
        _c_void_pp, ctypes.c_char_p, ctypes.c_size_t]

    lib.cmg_mesh_num_points.restype = ctypes.c_size_t
    lib.cmg_mesh_num_points.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_num_tets.restype = ctypes.c_size_t
    lib.cmg_mesh_num_tets.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_num_faces.restype = ctypes.c_size_t
    lib.cmg_mesh_num_faces.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_points.restype = _c_double_p
    lib.cmg_mesh_points.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_tets.restype = _c_int_p
    lib.cmg_mesh_tets.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_faces.restype = _c_int_p
    lib.cmg_mesh_faces.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_tet_markers.restype = _c_int_p
    lib.cmg_mesh_tet_markers.argtypes = [ctypes.c_void_p]
    lib.cmg_mesh_face_markers.restype = _c_int_p
    lib.cmg_mesh_face_markers.argtypes = [ctypes.c_void_p]

    lib.cmg_version.restype = ctypes.c_char_p


_lib = _load_lib()
_bind(_lib)
_BACKEND = "ctypes"


def _check(status: int, err: ctypes.Array) -> None:
    if status != 0:
        raise RuntimeError(err.value.decode() or f"cmg error {status}")


@dataclass
class Mesh:
    """A tetrahedral mesh returned across the C ABI as owning NumPy arrays."""

    points: np.ndarray          # (N, 3) float64
    tetrahedra: np.ndarray      # (M, 4) int32
    faces: np.ndarray           # (F, 3) int32
    tet_markers: np.ndarray     # (M,)   int32
    face_markers: np.ndarray    # (F,)   int32

    @property
    def tets(self) -> np.ndarray:
        """Alias for :attr:`tetrahedra` (backwards-compatible name)."""
        return self.tetrahedra


def _mesh_from_handle(out: ctypes.c_void_p) -> Mesh:
    """Copy the flattened C-side views into owning NumPy arrays, then free."""
    handle = out
    try:
        npts = _lib.cmg_mesh_num_points(handle)
        ntet = _lib.cmg_mesh_num_tets(handle)
        nfac = _lib.cmg_mesh_num_faces(handle)

        if npts:
            points = np.ctypeslib.as_array(
                _lib.cmg_mesh_points(handle), (npts, 3)).astype(np.float64).copy()
        else:
            points = np.empty((0, 3), dtype=np.float64)

        if ntet:
            tets = np.ctypeslib.as_array(
                _lib.cmg_mesh_tets(handle), (ntet, 4)).astype(np.int32).copy()
        else:
            tets = np.empty((0, 4), dtype=np.int32)

        if nfac:
            faces = np.ctypeslib.as_array(
                _lib.cmg_mesh_faces(handle), (nfac, 3)).astype(np.int32).copy()
        else:
            faces = np.empty((0, 3), dtype=np.int32)

        tet_markers = _markers(_lib.cmg_mesh_tet_markers(handle), ntet)
        face_markers = _markers(_lib.cmg_mesh_face_markers(handle), nfac)
    finally:
        _lib.cmg_mesh_destroy(handle)
    return Mesh(points=points, tetrahedra=tets, faces=faces,
                tet_markers=tet_markers, face_markers=face_markers)


def _markers(ptr, n: int) -> np.ndarray:
    if not n or not ptr:
        return np.empty((0,), dtype=np.int32)
    return np.ctypeslib.as_array(ptr, (n,)).astype(np.int32).copy()


class PLC:
    """A Piecewise-Linear Complex: the polyhedral input domain to mesh."""

    def __init__(self) -> None:
        self._handle = _lib.cmg_plc_create()
        if not self._handle:
            raise MemoryError("cmg_plc_create failed")

    def add_points(self, points: np.ndarray) -> "PLC":
        """Set the PLC vertex cloud from an (N, 3) array. Replaces any prior set."""
        pts = np.ascontiguousarray(points, dtype=np.float64)
        if pts.ndim != 2 or pts.shape[1] != 3:
            raise ValueError("points must be an (N, 3) array")
        _check(_lib.cmg_plc_set_points(
            self._handle, pts.ctypes.data_as(_c_double_p), pts.shape[0]),
            ctypes.create_string_buffer(1))
        return self

    def add_facet(self, vertex_indices: Sequence[int], marker: int = 0) -> "PLC":
        """Append a single-polygon facet (a triangle when three indices)."""
        idx = np.ascontiguousarray(vertex_indices, dtype=np.int32).ravel()
        if idx.size < 3:
            raise ValueError("a facet needs at least 3 vertex indices")
        _check(_lib.cmg_plc_add_facet(
            self._handle, idx.ctypes.data_as(_c_int_p), idx.size, int(marker)),
            ctypes.create_string_buffer(1))
        return self

    def add_hole(self, x: float, y: float, z: float) -> "PLC":
        """Append a volumetric hole seed point."""
        _check(_lib.cmg_plc_add_hole(self._handle, x, y, z),
               ctypes.create_string_buffer(1))
        return self

    def add_region(self, x: float, y: float, z: float,
                   attribute: float = 0.0, max_volume: float = -1.0) -> "PLC":
        """Append a material region seed with attribute and max-volume."""
        _check(_lib.cmg_plc_add_region(self._handle, x, y, z, attribute, max_volume),
               ctypes.create_string_buffer(1))
        return self

    def __del__(self) -> None:
        handle = getattr(self, "_handle", None)
        if handle:
            _lib.cmg_plc_destroy(handle)
            self._handle = None


@dataclass
class MeshOptions:
    """Typed meshing parameters mirroring the C++ ``cmg::MeshOptions``."""

    plc: bool = False
    max_volume: Optional[float] = None
    quality: Optional[tuple] = None  # (radius_edge, min_dihedral)
    preserve_edges: bool = False

    def _to_handle(self) -> ctypes.c_void_p:
        """Materialize a C ``cmg_options*`` reflecting these fields (caller frees)."""
        handle = _lib.cmg_options_create()
        if not handle:
            raise MemoryError("cmg_options_create failed")
        err = ctypes.create_string_buffer(1)
        _check(_lib.cmg_options_set_plc(handle, 1 if self.plc else 0), err)
        _check(_lib.cmg_options_set_preserve_edges(
            handle, 1 if self.preserve_edges else 0), err)
        if self.max_volume is not None:
            _check(_lib.cmg_options_set_max_volume(handle, self.max_volume), err)
        if self.quality is not None:
            radius_edge, min_dihedral = self.quality
            _check(_lib.cmg_options_set_quality(
                handle, radius_edge, min_dihedral), err)
        return handle


def tetrahedralize(plc: PLC, options: Optional[MeshOptions] = None) -> Mesh:
    """Boundary-conforming tetrahedralization of a PLC under ``options``."""
    if not isinstance(plc, PLC):
        raise TypeError("plc must be a cybermesh.PLC")
    opts = options if options is not None else MeshOptions(plc=True)
    opts_handle = opts._to_handle()
    out = ctypes.c_void_p()
    err = ctypes.create_string_buffer(256)
    try:
        st = _lib.cmg_tetrahedralize(
            plc._handle, opts_handle, ctypes.byref(out), err, 256)
        _check(st, err)
    finally:
        _lib.cmg_options_destroy(opts_handle)
    return _mesh_from_handle(out)


def delaunay(points: np.ndarray, options: Optional[MeshOptions] = None) -> Mesh:
    """Delaunay tetrahedralization of an (N, 3) point set."""
    pts = np.ascontiguousarray(points, dtype=np.float64)
    if pts.ndim != 2 or pts.shape[1] != 3:
        raise ValueError("points must be an (N, 3) array")
    opts = options if options is not None else MeshOptions()
    opts_handle = opts._to_handle()
    out = ctypes.c_void_p()
    err = ctypes.create_string_buffer(256)
    try:
        st = _lib.cmg_delaunay(
            pts.ctypes.data_as(_c_double_p), pts.shape[0], opts_handle,
            ctypes.byref(out), err, 256)
        _check(st, err)
    finally:
        _lib.cmg_options_destroy(opts_handle)
    return _mesh_from_handle(out)


def version() -> str:
    """Return the underlying C++ core version string."""
    return _lib.cmg_version().decode()


__all__ = ["PLC", "MeshOptions", "Mesh", "tetrahedralize", "delaunay", "version"]
