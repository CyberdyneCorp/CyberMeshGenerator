# file-formats Specification

## ADDED Requirements

### Requirement: Common ASCII file conventions

The `cmg::io` layer SHALL read and write ASCII TetGen files where a `#` begins a
comment to end-of-line, fields are separated by whitespace or commas, and objects
are numbered consecutively from 0 or 1. On read the index base SHALL be
auto-detected from the first data record and all indices normalized to 0-based
internally; on write the base SHALL follow the container's `index_base`. Malformed
input SHALL return a `MeshError`, never throw. (oracle: TetGen file-formats;
tetgen.cxx:2926-2948)

#### Scenario: Comments and separators ignored
- GIVEN a file whose lines contain `# …` comments and comma-separated fields
- WHEN it is read
- THEN the comment portions are skipped and comma or whitespace separation both parse

#### Scenario: Index base auto-detected
- GIVEN a `.node` file whose first point is index 1
- WHEN it is read
- THEN objects are treated as 1-based and normalized to 0-based indices internally

### Requirement: TetGen native geometry and mesh containers

The layer SHALL read and write `.node` (points with optional attributes and
boundary markers), `.poly` and `.smesh` (PLC: node section + facets + holes +
regions), `.ele` (tetrahedra with optional region attribute), `.face`
(triangular faces with optional marker), `.edge` (edges with optional marker), and
`.neigh` (per-tetrahedron neighbor quadruples, −1 for none), following the TetGen
header/record grammar. (oracle: TetGen file-formats §5.2)

#### Scenario: Node round-trip
- GIVEN a set of points with attributes and markers
- WHEN written to `.node` and read back
- THEN the points, attributes, and markers match

#### Scenario: PLC round-trip via .poly
- GIVEN a PLC with facets, a hole, and a region
- WHEN written to `.poly` and read back
- THEN the facets/polygons, hole seed, and region attribute/max-volume match

#### Scenario: Mesh containers round-trip
- GIVEN a `Mesh` of tetrahedra with faces and markers
- WHEN written to `.ele`/`.face` and read back (with the companion `.node`)
- THEN the tetrahedra, faces, and markers match

#### Scenario: Neighbor output
- GIVEN a mesh with neighbor adjacency
- WHEN written to `.neigh`
- THEN each tetrahedron line lists four neighbor indices with −1 on the boundary

### Requirement: Interchange surface formats

The layer SHALL read (as a `PLC` boundary) and write STL (ASCII and binary), OFF
(Geomview), PLY (ASCII), legacy VTK, and Medit `.mesh`. On STL read, coincident
vertices SHALL be merged so the resulting PLC has shared vertices. (oracle: TetGen
file-formats §5.3; tetgen.cxx load_off/load_ply/load_stl/load_medit/load_vtk)

#### Scenario: STL surface loaded with merged vertices
- GIVEN an STL file whose triangles repeat shared corner coordinates
- WHEN it is read
- THEN a PLC is produced whose coincident vertices are merged into shared indices

#### Scenario: OFF / PLY / VTK / Medit round-trip
- GIVEN a triangulated surface `PLC`
- WHEN written to each interchange format and read back
- THEN the vertex set and triangle connectivity match

### Requirement: Extension-based dispatch

The layer SHALL provide `read_points`, `read_plc`, `read_mesh`, `write_mesh`, and
`write_plc` that select the concrete format from the file extension, or from an
explicit `Format` argument, returning `cmg::expected<…, MeshError>`. An unknown or
unsupported extension SHALL return a `MeshError`. (oracle: TetGen input-object-type
detection)

#### Scenario: Dispatch by extension
- WHEN `read_plc("part.off")` is called
- THEN the OFF reader is selected and a `PLC` is returned

#### Scenario: Unknown extension errors cleanly
- WHEN a path with an unrecognized extension is passed
- THEN a `MeshError` is returned, not an exception
