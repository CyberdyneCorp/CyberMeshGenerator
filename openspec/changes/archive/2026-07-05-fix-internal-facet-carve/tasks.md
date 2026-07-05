# Tasks — Seed/flood-fill carve for internal facets

## Design
- [x] Confirm the flood-fill design against the code: hull-face = face owned by one tet; exterior seed = hull face not in constraint_faces; flood across non-constraint faces; keep the rest. Handle: no constraint faces / recovery incomplete -> fall back to ray cast; empty-domain guard
- [x] Decide where recovery-completeness is signalled to the carve (pipeline passes constraint faces only when recover_facets completed)

## Core
- [x] Add a flood-fill carve path in cdt::tetrahedralize_plc used when constraint_faces is non-null/complete: build face->tets adjacency, seed exterior from non-facet hull faces, BFS/DFS across non-constraint faces, keep non-exterior tets; else keep the current CarveGrid ray-cast path
- [x] Pipeline (src/core/tetrahedralize.cpp): pass constraint faces to the carve only when facet recovery completed; keep ray-cast otherwise
- [x] region::apply unchanged (already constraint-face aware) — verify holes still removed

## Tests (objective, must pass)
- [x] Square bipyramid split by an internal facet (needs a wall Steiner) + two region seeds: BOTH cells kept, volume conserved, two distinct markers, no straddling tet (this is the previously-FAILING case — must now pass)
- [x] Non-convex L-prism with preserve_facets: exact volume 12.0 unchanged (no regression)
- [x] Convex PLC with preserve_facets: identical mesh to the ray-cast result (no regression)
- [x] A hole seed inside a recovered domain still removes that component
- [x] No regression on the full suite (was 145)

## Verify
- [x] Adversarial: independently confirm the bipyramid volume is conserved (not 55% lost) and both markers present; confirm convex/non-convex unchanged; confirm no interior leak when a boundary facet is present
- [x] Green across default / -Werror / ASan / single precision / clang++; openspec validate --all --strict green
- [x] Update add-facet-recovery's documented non-goal / the code comment now that general internal-facet separation works (when recovery completes)

## Deferred
- [ ] Non-termination of conforming recovery on flat coplanar walls (a recovery issue, not the carve)
- [ ] Minimal-Steiner constrained Delaunay / `-Y`
