# Add region attributes and seed-based holes (Phase 6, increment 1)

## Why

A `PLC` can carry `regions` (a seed point + material attribute per sub-domain) and
`holes` (a seed point marking a void to remove). Phase 3 parsed and stored these
but ignored them. Multi-material meshes and seed-based void removal need them
honored. This change assigns material attributes to tetrahedra and removes
hole-seeded regions.

## Approach — post-carve connected components

After the interior mesh is carved (Phase 3), the kept tetrahedra form one or more
**connected components** in the face-adjacency graph. A component is a maximal set
of tetrahedra reachable across shared faces without leaving the mesh. Components are
separated wherever the domain is physically disconnected *or* wherever an internal
PLC facet appears in the mesh. This gives a robust classification that needs no
boundary recovery:

- **Region attribute (`-A`)**: the component containing a region seed takes that
  region's attribute; a tetrahedron never receives an attribute from across a
  component boundary (no diffusion across facets).
- **Automatic labels (`-AA`)**: with `MeshOptions::label_regions`, each component
  gets a distinct nonzero attribute even without seeds.
- **Hole seed**: the component containing a hole seed is removed.

## What changes

Spec delta for the **region-attributes** capability:

- `cmg::region::apply(mesh, plc, label_regions)`: builds the kept-tet adjacency
  components, removes hole-seeded components, assigns region-seed attributes into
  `Mesh::tet_markers`, optionally auto-labels unseeded components, and recomputes
  the domain boundary faces.
- The PLC tetrahedralization pipeline calls it when the PLC has regions or holes
  (or `label_regions` is set), so both the direct and refined paths honor them.
- `MeshOptions::label_regions` (the `-AA` analogue).

## Impact

- Two physically separated solids in one PLC, seeded with attributes 1 and 2, mesh
  into one `Mesh` whose tetrahedra carry the correct per-region attribute in
  `tet_markers` (and hence the `.ele` region column via Phase 2).
- A hole seed placed in one of them removes that solid's tetrahedra.

## Non-goals (deferred)

- **Regions separated only by an internal facet that is not present in the mesh** —
  distinguishing them requires constrained-Delaunay boundary/facet recovery
  (deferred with the CDT increment). This change classifies by connected component,
  which is exact when regions are distinct components (physically separated or
  separated by a facet the mesh already contains).
- **Per-region maximum-volume constraints** — needs region membership *during*
  refinement; a follow-up on top of this classification + Phase 4/5.
- **Exterior `-1` marking under `-c` (retain convex hull)** — niche; deferred.
