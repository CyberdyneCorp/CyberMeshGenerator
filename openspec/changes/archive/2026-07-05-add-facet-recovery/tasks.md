# Tasks — Facet recovery (conforming Delaunay, increment 1)

## Design / spike
- [x] Design doc: conforming facet recovery via Steiner insertion, mirroring recover_segments; DT-face test, split rule, termination/budget
- [x] Spike: recover ONE missing subface on a hand-built non-convex case before the full loop

## Core
- [x] `recover::plc_subfaces(plc)` — triangulated boundary + internal subfaces as index triples
- [x] `recover::recover_facets(points, subfaces, budget, segments)` — Steiner insertion until each subface is a DT face (or budget); deterministic; returns augmented points + completeness + leaf subfaces
- [x] Wire into the pipeline under `MeshOptions::preserve_facets` (after segment recovery, before carve); add the option
- [x] Constraint-face plumbing: recovered subfaces forwarded to region::apply so tets sharing one are not merged

## Tests (objective)
- [x] Non-convex domain (L-prism): every facet subface present as mesh faces; exact non-convex volume (12.0)
- [x] Non-Delaunay facet subface becomes a mesh face (prism cap)
- [x] Internal facet **that is already a mesh face** (bipyramid, zero Steiner) separates two regions, no straddling tet
- [x] Convex PLC with `preserve_facets`: zero Steiner added; byte-identical mesh
- [x] Budget exhaustion reports incompleteness (no infinite loop)
- [x] No regression: full suite green (was 139 non-facet baseline)

## Verify
- [x] Adversarial review: boundary recovery exact + volume conserved + determinism CONFIRMED; internal-facet separation over-claim caught and scoped
- [x] Green across default / -Werror / ASan / single precision / clang++; `openspec validate --all --strict` green

## Deferred (moved to non-goals after the adversarial verify)
- [ ] **General internal-facet region separation** — carve drops an interior cell (internal facets counted in its point-in-domain ray cast); conforming recovery non-terminating on a flat coplanar wall. Needs per-region carve (or excluding internal facets from the ray cast)
- [ ] Minimal-Steiner constrained Delaunay / flip-based recovery / `-Y` boundary-Steiner suppression
- [ ] Provable termination on acute / small-angle input (protecting balls)
- [ ] Curved / non-planar facets; general non-convex facet-polygon triangulation
