# region-attributes Specification (internal-facet separation delta)

## ADDED Requirements

### Requirement: Regions separated by an internal facet

CyberMeshGenerator SHALL, when a PLC's regions are separated only by an internal facet
(a facet interior to the domain, recovered under `preserve_facets`), assign each side the
attribute of the region seed it contains, keeping the sides as distinct components that no
tetrahedron crosses. This relies on the seed/flood carve (which keeps interior cells the
internal facet separates) and on constraint-face-aware component classification (two tets
sharing a recovered facet are not merged). (oracle: TetGen region marking across
subfaces)

#### Scenario: Two cells, two attributes
- GIVEN a PLC whose interior is split by one internal facet into two cells, a region seed
  with attribute 1 in one cell and attribute 2 in the other, meshed with `preserve_facets`
- WHEN region attributes are applied
- THEN the tetrahedra in each cell carry that cell's attribute, the two attributes are both
  present, and no tetrahedron spans the internal facet
