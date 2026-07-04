# Tasks — Full Python bindings (language-bindings)
- [x] Extend C ABI (bindings/c): cmg_plc_add_facet, add_hole, add_region; option setters (plc/quality/max_volume/preserve_edges/index_base); cmg_tetrahedralize; tet/face marker getters
- [x] Extend the pure-C smoke test to build+mesh a PLC through the new ABI
- [x] Extend the Python module (ctypes): PLC, MeshOptions, tetrahedralize, delaunay -> NumPy arrays
- [x] Python test: build a cube PLC, tetrahedralize, assert (N,3)/(M,4) arrays and volume ~1.0
- [x] `openspec validate --all --strict` green

## Deferred
- [ ] Swift wrapper completion (no toolchain here); nanobind wheel; voronoi/quality/coarsen in Python
