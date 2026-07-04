# Tasks — Binding file I/O
- [x] C ABI: cmg_read_plc/read_points/read_mesh/write_mesh/write_plc + cmg_plc_num_points (bindings/c), over cmg::io
- [x] C ABI: PLC geometry accessors cmg_plc_points/num_triangles/triangles (read a loaded PLC back)
- [x] Extend pure-C smoke to load a surface file and mesh it
- [x] Python: read_plc/read_points/read_mesh/write_mesh/write_plc over the C ABI
- [x] Python: PLC.points / PLC.triangles accessors (NumPy N×3 / M×3)
- [x] Swift: readPLC/readMesh/tetrahedralize/delaunay + PLC.points/.triangles over the C ABI (source-only; no Swift toolchain here)
- [x] Python test: write a small PLC to .obj/.stl, read it back, tetrahedralize; load and mesh
- [x] Python test: PLC accessors + native OBJ/STL/OFF load-and-mesh round-trip
- [x] Native-loading example (examples/native_load) — file → mesh with no hand-parser
- [x] `openspec validate --all --strict` green

## Deferred
- [ ] New formats (added elsewhere); streaming; format-specific options
