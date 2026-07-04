# tetgen-parity Specification

## Purpose
TBD - created by archiving change bootstrap-cybermesh-foundation. Update Purpose after archive.
## Requirements
### Requirement: TetGen capability parity roadmap

CyberMeshGenerator SHALL track parity with TetGen 1.6.0 as a prioritized backlog of
capabilities, each mirrored from TetGen's OpenSpec baseline and each graduating into
its own OpenSpec change ported against the TetGen oracle. The foundation change
SHALL NOT implement any of these capabilities; it records the ordering as the scope
guardrail. The tracked capabilities SHALL be, in dependency order:

1. **delaunay-tetrahedralization** — Bowyer-Watson incremental DT, weighted/regular
   DT, BRIO-Hilbert spatial sort, convex-hull output. (oracle:
   specs/delaunay-tetrahedralization)
2. **file-formats** — `.node`/`.poly`/`.smesh`/`.ele`/`.face`/`.edge`/`.vol`/`.var`,
   plus STL/OFF/PLY/VTK/Medit read and write. (oracle: specs/file-formats)
3. **constrained-tetrahedralization** — CDT of a PLC, segment and facet recovery,
   boundary-conforming Delaunay. (oracle: specs/constrained-tetrahedralization)
4. **quality-mesh-generation** — radius-edge and dihedral-angle bounds, global and
   per-region volume constraints, per-facet area / per-segment length, Steiner
   budget. (oracle: specs/quality-mesh-generation)
5. **adaptive-mesh-sizing** — background-mesh sizing function and metric-driven
   refinement. (oracle: specs/adaptive-mesh-sizing)
6. **region-attributes** — region marking, material attributes, hole carving,
   convex-hull retention. (oracle: specs/region-attributes)
7. **mesh-optimization** — vertex smoothing, flips, quality improvement, second-order
   nodes. (oracle: specs/mesh-optimization)
8. **mesh-coarsening** — vertex removal / decimation. (oracle: specs/mesh-coarsening)
9. **mesh-reconstruction** — re-mesh / refine an existing mesh, `.vol`/`.var`-driven
   refinement. (oracle: specs/mesh-reconstruction)
10. **voronoi-diagram** — Voronoi / power-diagram dual output. (oracle:
    specs/voronoi-diagram)
11. **command-line-interface** — optional TetGen-compatible `tetgen`-style
    executable and switch parser. (oracle: specs/command-line-interface)

#### Scenario: A capability graduates into its own change
- WHEN a roadmap capability is picked up
- THEN it is specified and implemented in its own OpenSpec change, ported against the
  TetGen oracle and its TetGen OpenSpec baseline, and archived on completion

#### Scenario: Foundation does not implement roadmap capabilities
- WHEN the foundation change is reviewed
- THEN it contains no meshing-capability implementation, only the skeleton, data
  model, integration contracts, predicate port, acceleration and binding
  architecture, and oracle harness

### Requirement: GPU acceleration is additive per capability

Each roadmap capability SHALL first land as a correct portable-CPU implementation
validated against the TetGen oracle; GPU device kernels for its parallelizable
phases SHALL be added through the backend-acceleration substrate as strictly
additive follow-up work, never as a prerequisite for correctness.

#### Scenario: CPU-correct before GPU
- WHEN a capability is delivered
- THEN its CPU path passes the oracle suite before any GPU kernel for that capability
  is enabled, and the GPU path is validated to match the CPU topology

### Requirement: Parity gaps are tracked, never silently dropped

CyberMeshGenerator SHALL record any TetGen behavior deliberately not ported (or
ported with a documented deviation) in this parity roadmap and in the responsible
change's `proposal.md` Non-goals, never left only in code comments.

#### Scenario: Deferred behavior is recorded
- WHEN a TetGen switch or behavior is intentionally deferred while implementing a
  capability
- THEN the deferral is added to this roadmap and the change's Non-goals in the same PR

