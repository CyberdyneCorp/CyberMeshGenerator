# Design — segment recovery

## Algorithm

```
recover_segments(points, segments, budget):
  work = list of segments as vertex-index pairs           # sub-segments to satisfy
  for round in 0..MAX_ROUNDS:
    mesh = delaunay(points)
    E = { sorted(u,v) : uv an edge of some tet of mesh }
    present, missing = partition(work, seg -> sorted(seg) in E)
    if missing empty: return {points, complete = true}
    next = present
    for seg=(a,b) in missing:
      if steiner_added >= budget: next.push(seg); continue     # give up (bounded)
      m = add point = midpoint(points[a], points[b])
      next.push((a,m)); next.push((m,b))                       # recurse on halves
    work = next
  return {points, complete = false}
```

- A segment is **satisfied** when it is exactly a Delaunay edge. A missing segment is
  bisected; its two halves join `work` and are checked next round. The original
  segment is thus recovered as a *chain* of mesh edges through the inserted
  midpoints.
- **Termination**: each bisection halves a sub-segment's length. Once a sub-segment
  is short enough relative to the local point spacing, the smallest sphere on it as
  diameter is empty, so it is a Delaunay edge. The `budget` and `MAX_ROUNDS` are hard
  backstops; an unrecovered segment under budget is reported via
  `complete = false` rather than looping.
- Re-checking **all** of `work` each round handles the case where inserting a
  midpoint for one segment perturbs another (adding a vertex can remove an existing
  Delaunay edge); the perturbed segment simply re-enters `missing` and is bisected.

## Edge extraction

Segments = the unique undirected edges of every facet polygon: for a polygon
`v0..v(n-1)`, the edges `(vi, v(i+1 mod n))`. Deduplicated as sorted index pairs so a
shared edge between two facets is recovered once.

## Integration

`MeshOptions::preserve_edges` gates it. When set and the PLC has facets,
`tetrahedralize(plc)`:

1. extracts the facet segments,
2. runs `recover_segments` to get an **augmented** point set (original points +
   edge Steiner points),
3. proceeds through the normal pipeline (carve, and refine if requested) using an
   augmented PLC — same facets (referencing the original corners), points replaced
   by the augmented set.

The Steiner points lie **on** facet edges (coplanar with their facets), so the
ray-cast carve — which tests against the original facet triangles — is unaffected;
the mesh simply now contains the feature edges. Recovery runs once, before
refinement.

## Testing

- **Missing segment recovered**: the z-axis + surrounding-triangle configuration
  where the axis segment is provably not Delaunay → after recovery it is covered by a
  chain of mesh edges (and a midpoint was added).
- **Already-Delaunay segments unchanged**: a cube's 12 edges are already Delaunay →
  recovery adds no points.
- **All facet edges present after meshing** with `preserve_edges`: for a PLC, every
  facet edge is a chain of mesh edges in the output.
- **Budget bound**: a demanding case caps the Steiner count and reports incomplete
  rather than looping.
- **Off by default**: without `preserve_edges`, the mesh is unchanged from before.
