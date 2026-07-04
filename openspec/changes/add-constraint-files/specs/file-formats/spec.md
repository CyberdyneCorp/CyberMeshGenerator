# file-formats Specification (constraint files delta)

## ADDED Requirements

### Requirement: `.vol` per-tetrahedron volume constraint file

CyberMeshGenerator SHALL read and write the `.vol` format: a header line `<#tets>`
followed by `<tet#> <max-volume>` lines, returning one maximum-volume value per
tetrahedron where a zero or negative value denotes "unconstrained". `read_vol` SHALL
take the expected tetrahedron count and error on a mismatch. (oracle: TetGen
file-formats §5.2.7; tetgen.cxx load_vol)

#### Scenario: .vol round-trip
- GIVEN a vector of per-tetrahedron maximum volumes
- WHEN written to `.vol` and read back with the same tetrahedron count
- THEN the values match

#### Scenario: Count mismatch is an error
- GIVEN a `.vol` file whose count differs from the expected tetrahedron count
- WHEN it is read
- THEN a `MeshError` is returned rather than a partial result

### Requirement: `.mtr` per-node sizing metric file

CyberMeshGenerator SHALL read and write the `.mtr` format: a header line
`<#nodes> <metric-size=1>` followed by one metric value per node (the target edge
length; 0 means the node size is ignored), returning one value per node. (oracle:
TetGen file-formats §5.2.8; tetgen.cxx)

#### Scenario: .mtr round-trip
- GIVEN a vector of per-node sizes
- WHEN written to `.mtr` and read back
- THEN the values match, one per node
