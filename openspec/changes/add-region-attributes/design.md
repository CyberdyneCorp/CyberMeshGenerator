# Design — region attributes and holes

## Connected-component classification

Run after the ray-cast carve (Phase 3) on the kept interior tetrahedra:

1. **Adjacency** — hash each tetrahedron face (sorted vertex triple) to the tets
   containing it; two kept tets sharing a face are adjacent.
2. **Components** — union-find (or BFS) over that adjacency yields the connected
   components of the interior mesh.
3. **Seed location** — for each region and hole seed, find the kept tetrahedron
   containing it (exact-`orient3d` barycentric test) and hence its component.
4. **Holes** — mark every component that contains a hole seed for removal.
5. **Attributes** — a component containing a region seed takes that region's
   attribute. With `label_regions` (`-AA`), each surviving unseeded component gets a
   distinct nonzero label. Otherwise unseeded components keep attribute 0.
6. **Rebuild** — drop removed components; write `tet_markers`; recompute boundary
   faces (a face bordering exactly one surviving tet, marker 1).

Why components rather than facet-flood: a component boundary is exactly where the
mesh has no neighbor across a face — i.e. a domain boundary or an internal facet
present in the mesh. So "no attribute diffusion across facets" holds for any regions
that are distinct components, with **no dependence on boundary recovery**. Regions
separated only by an internal facet the DT did not reproduce would merge into one
component — the documented deferral.

## Where it runs

`cmg::region::apply` is invoked inside the PLC pipeline right after carving, so:
- the **direct** path (`tetrahedralize(plc)`) gets attributes and hole removal;
- the **refined** path (Phase 4/5 re-meshes each round) removes hole components each
  round before continuing, so voids are never refined.

It is a no-op when the PLC has no regions/holes and `label_regions` is off, keeping
the plain convex-domain path unchanged.

## Robustness

Component construction and seed location use only integer adjacency and the exact
predicates, so classification is deterministic and robust. The only approximate
step remains the upstream ray-cast carve (unchanged). Volume conservation and the
single-precision caveat are inherited from Phase 3/4, not affected here.

## Testing

- **Single region**: a cube with one region seed (attribute 5) → every tetrahedron
  marked 5.
- **Two separated solids**: two disjoint cubes in one PLC, seeds with attributes 1
  and 2 → each solid's tetrahedra carry the right attribute (they are distinct
  components after the bridge is carved away).
- **Hole seed**: two cubes, a hole seed in one → that solid's tetrahedra removed,
  the other intact.
- **Auto-label**: two cubes, no seeds, `label_regions` on → two distinct nonzero
  labels, one per solid.
- **No-op**: a plain cube with no regions/holes → unchanged mesh, markers 0.
