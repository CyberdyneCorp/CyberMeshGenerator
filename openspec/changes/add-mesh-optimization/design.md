# Design — mesh optimization (Laplacian smoothing)

## Algorithm

1. **Classify vertices** — a vertex is on the boundary if it appears in any
   `mesh.faces` entry; all others are interior. Only interior vertices move.
2. **Adjacency** — from tetrahedron edges, build each vertex's set of edge-neighbors
   and its list of incident tetrahedra.
3. **Sweep** (repeat `iterations` times): for each interior vertex `v`,
   - compute `target` = centroid of `v`'s neighbors,
   - proposed = `v + relaxation·(target − v)`,
   - **inversion guard**: accept the proposed position only if every tetrahedron
     incident to `v` keeps the same `orient3d` sign as with the original position
     (evaluated with the exact predicate). If any would flip, damp the step (e.g.
     binary-search the fraction toward `target`) and re-test; if no positive fraction
     is safe, leave `v` put for this sweep.
   Updates are applied to a working copy so a sweep uses consistent positions.

Because boundary vertices are fixed and no tetrahedron inverts, the union of
tetrahedra (the domain) is unchanged, so total volume is invariant.

## Quality metric

`min_dihedral_angle` computes, per tetrahedron, the six dihedral angles (angle
between each pair of faces sharing an edge, from the face normals) and returns the
global minimum in degrees. Used to show smoothing does not worsen the worst angle.

## Testing

- Boundary vertices unchanged; every tet keeps its orientation sign (no inversion);
  total volume conserved within tolerance — on a refined cube mesh.
- `min_dihedral_angle` of the smoothed mesh ≥ that of the input.
- An interior vertex displaced from its neighbor centroid moves toward it after a
  full-relaxation sweep (when the move is inversion-safe).
