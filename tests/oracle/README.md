# TetGen oracle harness

CyberMeshGenerator is validated against **real TetGen 1.6.0** as the numerical
oracle, exactly as NumPP validates against NumPy and SciPP against SciPy. This
directory holds the oracle runner and its frozen golden data.

## How it works

A test states a point set / PLC and typed `MeshOptions`, then:

1. Runs TetGen (`/home/leonardo/work/TetGen`) on the **equivalent switch string**
   (`MeshOptions` maps 1:1 to TetGen switches).
2. Compares the two outputs under the equivalence criteria:
   - **Exact predicate signs** — identical for every shared orientation/in-sphere
     test.
   - **Topology in general position** — the set of tetrahedra as *sorted vertex
     tuples* matches TetGen's.
   - **Counts & quality** — vertex/tet/face counts, Steiner-point counts, and
     radius-edge / dihedral histograms match within documented tie-breaking.
   - **Invariants** — the output is a valid, non-overlapping tetrahedralization
     satisfying the (constrained) Delaunay property.

## Frozen mode (default in CI)

Set `CMG_ORACLE_FROZEN=1` (or `just oracle`) to compare against checked-in golden
meshes under `tests/oracle/golden/` instead of invoking TetGen, so CI runs with no
TetGen build. Regenerating the golden data is an explicit, reviewable step that
surfaces any divergence from the prior baseline before it is accepted.

## Degenerate inputs

For cospherical / coplanar inputs where the tetrahedralization is not
combinatorially unique, the harness applies the same symbolic-perturbation
tie-breaking TetGen uses; where insertion order still makes the exact tuple set
non-unique, it asserts validity + the Delaunay-property invariants instead of
exact-tuple equality, and the case is marked non-unique.

## Status

Phase 0 ships the criteria, the frozen-mode switch, and the zero-dependency runner
(`tests/harness.hpp`). The live TetGen-invoking comparison and the golden corpus
are populated alongside the Phase 1 `delaunay-tetrahedralization` change, when
there is a real kernel whose output to diff.
