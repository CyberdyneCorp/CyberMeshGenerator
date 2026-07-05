# constrained-tetrahedralization Specification (seed-carve delta)

## ADDED Requirements

### Requirement: Seed/flood-fill interior classification with recovered facets

CyberMeshGenerator SHALL classify tetrahedra as interior/exterior by flood fill (rather than per-centroid ray casting) when recovered facet subfaces are available as constraint faces (under `preserve_facets`, after facet recovery completes): it SHALL seed the exterior from every convex-hull face (a face owned by exactly one tetrahedron) that is not a constraint face, flood across faces that are not constraint faces, remove the exterior-reachable tetrahedra, and keep the rest. Interior cells separated by an internal
constraint facet SHALL all be kept (none dropped), and a convex or star-shaped domain
SHALL be unchanged from the ray-cast result (no hull face is a non-facet, so nothing is
seeded). When `preserve_facets` is off or facet recovery did not complete, it SHALL fall
back to the ray-cast carve. (oracle: TetGen carveholes; manual §4.2.2)

#### Scenario: An internal facet separates two regions (recovery completes)
- GIVEN a PLC split by one internal facet whose recovery completes (e.g. a square bipyramid
  needing a wall Steiner point), with a region seed on each side
- WHEN it is tetrahedralized with `preserve_facets`
- THEN both cells are kept, the total volume is conserved, the two cells receive distinct
  region markers, and no tetrahedron straddles the facet

#### Scenario: Non-convex boundary is unchanged
- GIVEN a non-convex domain (e.g. an L-shaped prism) meshed with `preserve_facets`
- WHEN the seed carve runs
- THEN every boundary facet subface is a mesh face and the exact non-convex volume is
  conserved (same result as before this change)

#### Scenario: Convex domain is unchanged
- GIVEN a convex PLC meshed with `preserve_facets`
- THEN no exterior tetrahedra are seeded and the kept mesh equals the ray-cast result
