# Seed/flood-fill carve so internal facets separate regions

## Why

The PLC carve keeps a tetrahedron when its centroid is "inside the domain", tested by ray
casting against **every** facet triangle (`CarveGrid` over `boundary_triangles`). An
**internal** facet (a wall with domain on both sides) is a single surface, so a ray from
an interior point crosses it and flips the parity — the carve then drops the cell on one
side. This is the pre-existing defect the facet-recovery adversarial pass surfaced:
adding an internal wall drops ~55 % of the volume and collapses two regions into one, even
though the facet is correctly recovered as a mesh face. It also blocks the region-attributes
deferral "regions separated only by an internal facet".

You cannot tell a priori which facets are internal, so excluding them from the ray cast is
not possible. TetGen's actual approach is a **seed/flood carve**, which this change adopts.

## What changes

Spec delta for **constrained-tetrahedralization** (and it satisfies a **region-attributes**
deferral). When the recovered facet subfaces are available as constraint faces (i.e. under
`preserve_facets`, once recovery is complete), classify interior/exterior by flood fill
instead of the per-centroid ray cast:

- Build tetrahedron adjacency across faces that are **not** constraint faces.
- Seed the exterior from every convex-hull face (a face owned by exactly one tet) that is
  **not** a domain facet (not a constraint face); flood across non-constraint faces to mark
  all exterior-reachable tets.
- Keep the tets not marked exterior. Because internal facets are constraint walls, interior
  cells they separate are never reached from outside — none are dropped — and
  `region::apply` (already constraint-face aware) gives each cell its region seed's marker.
- Convex/star-shaped domains are unchanged: every hull face is a domain facet, so there are
  no exterior seeds and all tets are kept (identical to today).
- When `preserve_facets` is off, or recovery did not complete (constraint faces would be
  incomplete and the flood could leak), fall back to the existing ray-cast carve.

## Impact

- A domain split by an internal facet (that recovery completes) meshes with **both** cells
  present, distinct region markers, volume conserved — the previously-failing square
  bipyramid now works.
- Non-convex boundary recovery (L-prism) and convex domains are unchanged (no regression).
- Removes the "general internal-facet region separation" non-goal added by add-facet-recovery.

## Non-goals
- **Non-termination of conforming recovery on flat coplanar walls** (e.g. a box split by an
  axis-aligned wall of cospherical points) — that is a *recovery* limitation, not a carve
  one; the carve fix applies once recovery completes. Well-conditioned internal walls
  (bipyramid, oblique walls) complete and are covered.
- **Minimal-Steiner constrained Delaunay** / `-Y`.
- Changing the carve for the non-recovered (ray-cast) path — it stays as the fallback for
  convex/star-shaped input without `preserve_facets`.
