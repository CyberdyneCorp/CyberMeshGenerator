# file-formats Specification (OBJ delta)

## ADDED Requirements

### Requirement: Wavefront OBJ read/write

CyberMeshGenerator SHALL read and write Wavefront OBJ surfaces: `read_obj(path)`
parses `v` vertex lines and `f` face lines (accepting the `v`, `v/vt`, `v/vt/vn`, and
`v//vn` corner forms and any polygon size), returning a `PLC` whose vertices are the
OBJ vertices and whose facets are the OBJ faces; `write_obj`/`write_obj_mesh` emit `v`
and `f` lines. OBJ SHALL be selected for the `.obj` extension by the dispatcher.
Material and normal data MAY be ignored. (oracle: TetGen third-party formats; the
Wavefront OBJ spec)

#### Scenario: OBJ round-trip
- GIVEN a PLC of a triangulated surface
- WHEN written to `.obj` and read back
- THEN the vertex set and face connectivity match

#### Scenario: OBJ face index forms
- GIVEN an OBJ whose faces use `f a/ta b/tb c/tc` (vertex/texture) corners
- WHEN it is read
- THEN each face's vertex indices are parsed correctly (the texture indices are
  ignored) and returned as a facet

#### Scenario: Dispatch by extension
- WHEN `read_plc("model.obj")` is called
- THEN the OBJ reader is selected and a PLC is returned
