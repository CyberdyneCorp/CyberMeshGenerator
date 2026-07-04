# Add Delaunay tetrahedralization (Phase 1)

## Why

The foundation (bootstrap-cybermesh-foundation) established the typed API, the
robust predicates, and the build/backend/binding architecture, but returns
`NotImplemented` for any point set larger than a single tetrahedron. Delaunay
tetrahedralization (DT) is the kernel every other TetGen capability is built on —
constrained meshing, quality refinement, and the Voronoi dual all start from a
DT. This change implements it.

It ports TetGen's default operation: computing the Delaunay tetrahedralization of a
3-D point set via incremental Bowyer-Watson insertion, the BRIO-Hilbert spatial
sort that precedes insertion, the weighted (regular) Delaunay variant, and the
convex-hull face output. (oracle: TetGen `delaunay-tetrahedralization` spec;
tetgen.cxx incremental insertion)

## What changes

Spec delta for the **delaunay-tetrahedralization** capability:

- **Incremental Bowyer-Watson DT** of an n-point set (n ≥ 4), producing a valid
  tetrahedralization whose every tetrahedron satisfies the empty-circumsphere
  property, using the ported exact `orient3d` / `insphere` predicates for all
  decisions so degenerate (cospherical / coplanar) inputs are handled robustly.
- **BRIO-Hilbert spatial sort** of the insertion order (biased randomized
  insertion order over Hilbert-curve-sorted rounds), controllable and disableable,
  to give insertion locality and near-linear expected performance.
- **Weighted (regular) Delaunay** (`MeshOptions::weighted`): each point carries a
  weight; insertion uses the lifted paraboloid / `orient4d` power test, and
  redundant (dominated) points are retained in the point list but belong to no
  tetrahedron.
- **Convex-hull faces**: the boundary faces of the DT (each marked 1) are emitted
  in `Mesh::faces`, matching TetGen's default `.face` output for a point set.
- **Adjacency** (`MeshOptions::emit_neighbors`): populate `Mesh::neighbors` with
  the four face-neighbors per tetrahedron (−1 on the hull).

## Impact

- `cmg::delaunay()` handles the general n-point case (removing the foundation's
  `NotImplemented` stub); `cmg::tetrahedralize()` on a point-only PLC routes to it.
- The accelerable phases (Hilbert-key computation, point location, batched
  in-sphere filtering) are wired to the backend-dispatch shim with CPU fallbacks;
  actual GPU device kernels remain Phase 11.
- Validated against the TetGen oracle: identical exact predicate signs, and
  topology equal to TetGen's in general position (empty-circumsphere invariants +
  hull equality where the triangulation is non-unique).

## Non-goals

- **No** constrained Delaunay / PLC boundary recovery — that is Phase 3.
- **No** quality refinement or Steiner points — Phase 4.
- **No** GPU device kernels — the phases are routed through the dispatch shim but
  the CUDA/OpenCL/Metal kernels themselves are Phase 11.
- **No** mesh file I/O — reading `.node` and writing `.ele`/`.face` is Phase 2
  (`file-formats`); this change works on in-memory `Point3` arrays.
- **No** Voronoi dual output — Phase 9.

Delivered with documented partial scope (tracked in `tasks.md`, to be completed in
a follow-up before this capability is archived):

- **Duplicate collapse** handles exact coincidence; within-`coplanar_tolerance`
  merging of near-duplicates is deferred.
- **Sort control** exposes enable/disable (`spatial_sort`) and a deterministic
  `sort_seed`; the full TetGen `-b` threshold/ratio/Hilbert-order knobs are deferred.
