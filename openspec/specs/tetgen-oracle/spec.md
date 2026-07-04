# tetgen-oracle Specification

## Purpose
TBD - created by archiving change bootstrap-cybermesh-foundation. Update Purpose after archive.
## Requirements
### Requirement: TetGen 1.6.0 as the numerical oracle

CyberMeshGenerator's test harness SHALL validate results against real TetGen 1.6.0
(`/home/leonardo/work/TetGen`) as the oracle, mirroring NumPP↔NumPy and SciPP↔SciPy.
A test SHALL state inputs and typed options, run TetGen on the equivalent switch
string, and assert equivalence of the outputs. Spec requirements across the project
SHALL cite the ported TetGen source as a breadcrumb (e.g. `(oracle: tetgen.cxx:3266)`,
`(oracle: predicates.cxx)`, `(oracle: manual §5.2.1)`).

#### Scenario: Result compared against TetGen
- GIVEN a point set or PLC and options
- WHEN CyberMeshGenerator meshes it and TetGen meshes the equivalent switch string
- THEN the harness asserts the outputs are equivalent under the rules below

### Requirement: Topological and numeric equivalence criteria

The harness SHALL compare outputs by: (a) **exact predicate signs** — identical for
every shared orientation/in-sphere test; (b) **topology in general position** — the
set of tetrahedra as sorted vertex-index tuples SHALL match TetGen's; (c) **counts
and quality** — vertex/tet/face counts, Steiner-point counts, and quality histograms
SHALL match within documented tie-breaking; and (d) **invariants** — the output
SHALL satisfy the Delaunay (or constrained-Delaunay) property and be a valid,
non-overlapping tetrahedralization.

#### Scenario: General-position topology matches
- GIVEN a point set in general position
- WHEN both tools compute the Delaunay tetrahedralization
- THEN the sets of tetrahedra match as sorted vertex tuples

#### Scenario: Quality histogram matches
- GIVEN a PLC meshed with a quality bound
- WHEN both tools refine it
- THEN the radius-edge / dihedral-angle histograms match within documented tolerance

### Requirement: Degenerate-input tie-breaking

The harness SHALL, for degenerate inputs (cospherical or coplanar points) where the
tetrahedralization is not combinatorially unique, apply the same
symbolic-perturbation tie-breaking TetGen uses; where insertion order still makes
the exact tuple set non-unique, the harness SHALL assert validity and the
Delaunay-property invariants rather than exact-tuple equality, and the case SHALL be
documented as such. (oracle: tetgen.cxx symbolic perturbation)

#### Scenario: Cospherical points resolved consistently
- GIVEN five cospherical points
- WHEN both tools tetrahedralize them
- THEN either the tuple sets match under shared tie-breaking, or both outputs are
  asserted valid and Delaunay, with the case marked non-unique

### Requirement: Frozen oracle mode for CI

The harness SHALL support a frozen/checked mode that serializes reference meshes
produced by TetGen into `tests/golden/`, so CI can run the oracle comparisons
without building or invoking TetGen. Regenerating the golden data SHALL be an
explicit, reviewable step that surfaces any divergence from a prior baseline.

#### Scenario: CI runs without TetGen
- GIVEN a CI environment with no TetGen build
- WHEN the oracle test suite runs in frozen mode
- THEN comparisons use the checked-in golden meshes and pass without invoking TetGen

#### Scenario: Divergence surfaces on regeneration
- WHEN the golden data is regenerated and a mesh differs from the prior baseline
- THEN the difference is reported for review before the new baseline is accepted

### Requirement: Regression tests for fixed bugs

Every bug fix SHALL add a regression test reproducing the bug and asserting the
corrected behavior against the TetGen oracle (or against a documented, agreed
correct result where TetGen itself is wrong), per project rules.

#### Scenario: Bug fix carries a regression test
- WHEN a meshing bug is fixed
- THEN the fixing change adds a test that fails before the fix and passes after,
  wired into the oracle harness

