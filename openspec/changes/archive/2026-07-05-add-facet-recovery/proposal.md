# Add facet recovery (conforming Delaunay) — increment 1

## Why

The PLC carve keeps tetrahedra by centroid classification, so a PLC **facet** appears in
the output only when it happens to be a face of the Delaunay tetrahedralization of the
vertices. For non-convex domains and for **internal** facets (walls separating two
regions) that is not guaranteed — the boundary is approximated and region-separating
facets can be missing. Segment (feature-edge) recovery already ships via **conforming
Delaunay Steiner insertion** (`recover::recover_segments` adds points until each segment
is a chain of Delaunay edges). This change extends the *same, proven approach* to facets.

This is the **tractable** half of the deferred CDT work. The minimal-Steiner
constrained-Delaunay recovery (flip-based, `-Y` boundary-Steiner suppression) stays a
documented non-goal — it is much harder and is a later increment.

## What changes

Spec delta for **constrained-tetrahedralization** plus a small recovery module:

- **`recover::recover_facets(points, subfaces, budget)`** — insert Steiner points
  (encroached-subface splitting / subface circumcenters projected on the facet plane)
  until every required subface is a face of the Delaunay tetrahedralization of the
  augmented point set, or the budget is exhausted. Mirrors `recover_segments`.
- **`recover::plc_subfaces(plc)`** — the triangulated subfaces of every facet (boundary
  and internal) as index triples.
- **Pipeline**: under `MeshOptions::preserve_facets` (implying segment recovery first, so
  facet edges are present before their interiors), run facet recovery before the carve so
  the kept mesh's faces conform to the PLC facets and internal facets are present.

## Impact

- A non-convex PLC meshes with every **boundary** facet subface present as a union of mesh
  faces, and the boundary lies on the PLC facets (not an approximation) — volume conserved
  exactly (verified on an L-shaped prism: 36 subfaces recovered, volume 12.0 to 1e-6).
- An internal facet that recovery leaves as a mesh face (e.g. one already Delaunay, needing
  no wall Steiner points) separates the two sides into distinct regions.
- Existing convex/star-shaped behaviour is unchanged (nothing to recover → no Steiner
  points added; byte-identical mesh).

## Non-goals
- **General internal-facet region separation** — when recovering an internal wall needs
  Steiner points on it, the carve drops an interior cell (it counts internal facets in its
  point-in-domain ray cast) and conforming recovery does not terminate on a flat wall of
  coplanar points. Deferred to a follow-up that carves per-region (or excludes internal
  facets from the ray cast). This increment delivers **boundary** facet recovery robustly;
  internal-facet separation only for facets that are already mesh faces.
- **Minimal-Steiner constrained Delaunay** / flip-based recovery / `-Y` boundary-Steiner
  suppression — conforming recovery adds more Steiner points but is robust and
  terminating; minimal-Steiner is a later increment.
- **Provable termination on acute / small-angle input** (protecting-ball schemes).
- **Curved or non-planar facets**; **general non-convex facet-polygon triangulation**
  beyond fan triangulation of the given polygons.
