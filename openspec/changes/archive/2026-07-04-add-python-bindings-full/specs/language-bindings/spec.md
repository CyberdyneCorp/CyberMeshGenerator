# language-bindings Specification (full Python delta)

## ADDED Requirements

### Requirement: C ABI covers PLC construction, options, and outputs

The stable C ABI SHALL expose building a PLC (points plus facets via
`cmg_plc_add_facet`, and hole/region seeds), setting the mesh options that the typed
API supports (PLC mode, quality, maximum volume, edge preservation, index base),
`cmg_tetrahedralize`, and reading back the mesh arrays including tetrahedron and face
markers — so a binding can drive the full meshing surface without touching C++.
(oracle: language-bindings; mesh-core-foundation)

#### Scenario: PLC meshed through the C ABI
- GIVEN a PLC built through the C ABI (points + facets) and options with `plc` set
- WHEN `cmg_tetrahedralize` is called
- THEN a mesh handle is returned whose tetrahedron and face arrays are readable

### Requirement: Python meshing with NumPy interop

The Python module SHALL expose `PLC`, `MeshOptions`, `tetrahedralize`, and `delaunay`
over the C ABI, returning mesh points as an `(N,3)` NumPy array and tetrahedra as an
`(M,4)` NumPy array. Building and meshing a PLC from Python SHALL produce a valid mesh.
(oracle: language-bindings; NumPy interop)

#### Scenario: Cube PLC meshed from Python
- GIVEN a unit-cube `PLC` built in Python and `MeshOptions(plc=True)`
- WHEN `tetrahedralize` is called
- THEN `mesh.points` is an `(N,3)` array, `mesh.tetrahedra` is an `(M,4)` array, and the
  summed tetrahedron volume is ~1.0
