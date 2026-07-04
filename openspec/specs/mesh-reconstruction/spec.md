# mesh-reconstruction Specification

## Purpose
TBD - created by archiving change add-mesh-reconstruction. Update Purpose after archive.
## Requirements
### Requirement: Refine an existing mesh

CyberMeshGenerator SHALL provide `cmg::reconstruct::reconstruct(mesh, opts)` that
re-tetrahedralizes the input mesh's vertex set and applies the refinement and sizing
options in `opts`, returning a valid tetrahedral mesh over the same vertices (plus
any Steiner points) that meets the new constraints. The input vertices SHALL all be
retained. (oracle: TetGen mesh-reconstruction, `-r`; manual §4.2.5)

#### Scenario: Reconstruct and refine to a finer volume
- GIVEN an existing coarse mesh and `opts.max_volume` smaller than its largest tet
- WHEN `reconstruct(mesh, opts)` is called
- THEN the result is a valid mesh over the same (and additional Steiner) vertices
  whose tetrahedra all meet the volume bound

#### Scenario: No options reproduces the Delaunay mesh
- GIVEN a mesh and default options
- WHEN it is reconstructed
- THEN the result is the Delaunay tetrahedralization of its vertices and retains
  every input vertex

