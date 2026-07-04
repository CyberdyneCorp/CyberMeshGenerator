# Design — adaptive mesh sizing

## Sizing function

The single abstraction is a scalar field `h(p)` = desired edge length at point `p`,
represented as `std::function<double(const Point3&)>` on `MeshOptions::sizing`. A
return value ≤ 0 means "unconstrained here" (TetGen's size-0 convention). This one
type covers both TetGen sizing sources:

- **Programmatic** (`tetunsuitable`): the caller supplies any closure.
- **Background mesh** (`-m` / `.mtr`): `cmg::sizing::from_background` wraps a
  background mesh + per-node sizes into a closure.

## Mapping size to a volume target (why volume, not edge length)

Refinement reuses Phase 4's volume engine. A tetrahedron is "too big" at its
centroid `c` when its volume exceeds the target volume of a regular tetrahedron
with edge `h(c)`:

```
target_volume(c) = h(c)^3 / (6 * sqrt(2))
bad(t) = volume(t) > target_volume(centroid(t))          [when h(c) > 0]
```

Using volume (not longest edge) is deliberate: it is **self-limiting** — slivers
have tiny volume and are never targeted — so refinement terminates and does not
diverge, exactly as in Phase 4. An edge-length criterion would re-introduce the
sliver blow-up Phase 4 documented.

When both `sizing` and a global `max_volume` are set, the per-tet target is the
**tighter** of the two: `min(max_volume, target_volume(c))`.

## Background-mesh interpolation

`from_background(bg, node_sizes, scale)` returns `h(p)`:

1. Locate `p` in `bg` — find a tetrahedron containing `p` by the exact-`orient3d`
   barycentric sign test (linear scan this increment; a spatial index is a later
   optimization).
2. Barycentrically interpolate the four node sizes at `p`, multiply by `scale`.
3. If `p` is outside `bg` (no containing tet), return the nearest node's size (so
   points just outside the background domain still get a sensible size) — or 0 to
   leave them unconstrained; we use nearest-node for graceful behavior.

## Wiring

`refine()` gains an optional sizing function. `bad(t)` becomes the volume test
against the per-tet target. `delaunay()` / `tetrahedralize()` treat a mesh as
refinement-requested when `sizing` **or** `max_volume` is set. Everything else
(circumcenter/centroid insertion, Steiner budget, domain carving, termination) is
unchanged from Phase 4.

## Testing

- **Graded analytic sizing**: a box with `h = 0.15` for `x < 0.5` and `0.6`
  otherwise → mean tetrahedron volume in the fine half is markedly smaller than in
  the coarse half, and fine-half tets meet the tighter target.
- **Uniform analytic sizing** reproduces a near-uniform mesh comparable to the
  equivalent global `max_volume`.
- **Background-mesh sizing**: a background mesh with small node sizes in one region
  drives finer refinement there; interpolation is exact at background nodes.
- **Combined with `max_volume`**: the tighter target wins.
- **No sizing + no volume** ⇒ unrefined (unchanged Phase 1–3 mesh).
