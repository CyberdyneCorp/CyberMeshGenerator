# region-attributes Specification

## Purpose
TBD - created by archiving change add-region-attributes. Update Purpose after archive.
## Requirements
### Requirement: Region attribute assignment from seeds

CyberMeshGenerator SHALL assign to each tetrahedron the attribute of the PLC region
whose seed lies in the same connected component of the interior mesh, writing it to
`Mesh::tet_markers`. An attribute SHALL NOT diffuse across a component boundary: every
tetrahedron in one connected component receives the same attribute. Tetrahedra in a
component with no region seed SHALL receive attribute 0 (unless auto-labeling is
enabled). (oracle: TetGen region-attributes, `-A`; manual §4.2.9)

#### Scenario: Two separated materials
- GIVEN a PLC of two disjoint solids with region seeds carrying attributes 1 and 2
- WHEN it is tetrahedralized
- THEN every tetrahedron of the first solid is marked 1 and every tetrahedron of the
  second is marked 2

#### Scenario: Single region attribute fills the domain
- GIVEN a convex PLC with one region seed of attribute 5
- WHEN it is tetrahedralized
- THEN every tetrahedron carries attribute 5

### Requirement: Seed-based hole removal

CyberMeshGenerator SHALL remove every tetrahedron in a connected component that
contains a PLC hole seed. (oracle: TetGen holes; manual §5.2.2 Part 3)

#### Scenario: Hole seed carves a solid away
- GIVEN a PLC of two disjoint solids and a hole seed inside the first
- WHEN it is tetrahedralized
- THEN the first solid's tetrahedra are removed and the second solid's are kept

### Requirement: Automatic region labeling

Under `MeshOptions::label_regions`, CyberMeshGenerator SHALL assign a distinct
nonzero attribute to each surviving connected component even when no region seeds are
supplied, so tetrahedra in the same component share an attribute and different
components differ. (oracle: TetGen `-AA`; CHANGELOG v1.4.0)

#### Scenario: Auto-labeled components
- GIVEN a PLC of two disjoint solids with no region seeds and `label_regions` set
- WHEN it is tetrahedralized
- THEN the two solids receive two distinct nonzero attributes

### Requirement: No regions means unchanged attributes

CyberMeshGenerator SHALL leave tetrahedron attributes at 0 and the mesh unchanged
when the PLC declares no regions or holes and `label_regions` is off.

#### Scenario: Plain domain unaffected
- GIVEN a PLC with no regions or holes and `label_regions` off
- WHEN it is tetrahedralized
- THEN all `tet_markers` are 0 and no tetrahedra are removed by classification

