# Tasks — Segment recovery (CDT boundary recovery, step 1)

## Recovery engine
- [x] `cmg::recover::recover_segments(points, segments, budget)` (include/cmg/recover + src/recover)
- [x] Mesh-edge set extraction; missing-segment detection
- [x] Bisection: midpoint insertion, recurse on halves; re-check all segments each round
- [x] Budget + MAX_ROUNDS bound; `complete` flag on the result
- [x] Facet-edge extraction (unique undirected polygon edges)
- [x] `MeshOptions::preserve_edges`; augment the PLC and route the pipeline through it
- [x] Wire into CMake

## Tests
- [x] Non-Delaunay segment (z-axis + triangle) recovered as an edge chain (+ Steiner added)
- [x] Cube edges already Delaunay → no Steiner points
- [x] PLC with `preserve_edges`: every facet edge covered by a mesh-edge chain
- [x] Budget bounds a hard case; reports incomplete without looping
- [x] `preserve_edges` off ⇒ unchanged mesh
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred (remaining CDT work)
- [ ] Facet (triangle interior) recovery — the harder half
- [ ] Flip-based CDT with minimal Steiner points
- [ ] Small-angle / acute-input protecting-ball scheme
