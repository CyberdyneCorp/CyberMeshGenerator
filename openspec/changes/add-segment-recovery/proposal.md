# Add segment (feature-edge) recovery — first step of CDT boundary recovery

## Why

Boundary recovery — forcing a PLC's segments and facets to appear in the mesh — is
the core of a true constrained Delaunay tetrahedralization and the recurring root of
several deferrals (exact facet preservation, internal-facet region separation,
shape-refinement encroachment). It splits into two halves: **segment recovery**
(edges) and **facet recovery** (triangles). This change delivers the first,
tractable half: segment recovery.

A PLC facet edge is a *feature edge* of the geometry (a crease). The Delaunay
tetrahedralization of the vertices does not always contain it — the DT may connect
other vertices across the feature instead. This change recovers such edges so they
appear in the mesh, which matters for finite-element meshing of models with sharp
features and is a prerequisite for facet recovery.

Empirically confirmed: for two points on the z-axis with a triangle of points around
the origin, the axis segment is provably **not** a Delaunay edge (no empty
circumsphere passes through both endpoints); inserting its midpoint recovers it as a
two-edge chain.

## Approach — bisection (conforming)

For each required segment `(a,b)` that is not a mesh edge, insert its midpoint as a
Steiner point and recover the two halves recursively. Bisection terminates: as the
point set densifies, short sub-segments acquire an empty circumsphere and become
Delaunay edges. A Steiner budget bounds the worst case.

## What changes

Spec delta adding to the **constrained-tetrahedralization** capability:

- `cmg::recover::recover_segments(points, segments, budget)`: inserts Steiner points
  by bisection until every required segment is covered by a chain of Delaunay edges;
  returns the augmented point set and whether recovery completed.
- `MeshOptions::preserve_edges`: when set, `tetrahedralize(plc)` recovers the PLC's
  facet edges (via the augmented point set) before carving, so every facet edge
  appears as a chain of mesh edges. Off by default (unchanged behavior).

## Impact

- With `preserve_edges`, a PLC whose feature edge is not initially Delaunay is meshed
  with that edge present as a chain of mesh edges. A cube (whose edges are already
  Delaunay) is unchanged (no Steiner points added).
- Composes with carving and refinement: recovery runs first, then the existing
  pipeline.

## Non-goals (deferred — the remaining CDT work)

- **Facet (triangle) recovery** — forcing PLC facet *interiors* to appear as mesh
  faces. This is the harder half (coplanar-facet degeneracies) and the next
  increment; segment recovery is its prerequisite.
- **Flip-based CDT with minimal Steiner points** — this change uses conforming
  (Steiner) recovery, which is robust and terminating but not Steiner-minimal.
- **Small-angle / acute-input protection** (segment splitting near sharp dihedral
  angles can require many points) — bounded by the budget; a dedicated
  protecting-ball scheme is deferred.
