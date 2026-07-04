# Tasks — Wavefront OBJ format
- [x] `read_obj` / `write_obj` / `write_obj_mesh` (src/io/obj.cpp) — parse v/f (all corner forms, any polygon), fan-triangulate to facets; write v/f
- [x] Dispatcher: `.obj` -> Obj in detect()/read_plc()/write_plc()/write_mesh()
- [x] Tests: OBJ round-trip; `v/vt/vn` corner parsing; dispatch by extension
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Materials/textures (.mtl, vt/vn semantics); negative indices; free-form geometry
