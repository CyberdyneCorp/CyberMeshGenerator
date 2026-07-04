# robust-geometric-predicates Specification

## ADDED Requirements

### Requirement: Exact adaptive predicates ported verbatim

CyberMeshGenerator SHALL provide Jonathan Shewchuk's adaptive-precision
floating-point predicates — `orient3d`, `insphere`, `orient2d`, `incircle` —
ported from TetGen's `predicates.cxx` with minimal change (namespacing and the
`REAL` typedef only), not rewritten. Each predicate SHALL first evaluate a fast
floating-point estimate with a static/dynamic error filter and escalate to exact
arithmetic only when the sign is uncertain, guaranteeing the correct sign for any
input. (oracle: predicates.cxx; manual §3.1.3)

#### Scenario: Correct sign on near-degenerate configuration
- GIVEN four nearly-coplanar points
- WHEN they are tested by `orient3d`
- THEN the predicate returns the exact sign of the orientation, not a rounded estimate

#### Scenario: Sign identical to TetGen
- GIVEN any point configuration also fed to TetGen's predicate
- WHEN `orient3d` / `insphere` are evaluated on both
- THEN CyberMeshGenerator returns the identical sign as the TetGen oracle

### Requirement: One-time predicate initialization

CyberMeshGenerator SHALL initialize the predicates once at startup (an `exactinit`
equivalent) to compute the machine-epsilon-dependent error bounds before any
predicate is evaluated, and this initialization SHALL be thread-safe and idempotent.
(oracle: predicates.cxx `exactinit`)

#### Scenario: Predicates usable after initialization
- WHEN the library is loaded and `exactinit` has run
- THEN every predicate evaluation uses the correct machine-epsilon error bounds

#### Scenario: Safe concurrent first use
- WHEN two threads evaluate a predicate for the first time simultaneously
- THEN initialization happens exactly once and both evaluations are correct

### Requirement: Predicate build isolation at `-O0`

The predicates SHALL be compiled in their own translation unit
(`src/predicates/predicates.cpp`) at optimization level `-O0` (via
`set_source_files_properties` or equivalent) while the rest of the library is
compiled at high optimization, preventing floating-point contraction/reassociation
from breaking the adaptive error analysis. This is a correctness invariant, not a
performance tuning choice. (oracle: makefile `PREDCXXFLAGS = -O0`; manual §3.1.3)

#### Scenario: Predicates unaffected by the optimizer
- WHEN the library is built with `-O3`
- THEN the predicates translation unit is still built with `-O0` so its adaptive
  error analysis remains valid

### Requirement: Coplanarity tolerance control

CyberMeshGenerator SHALL expose a typed coplanarity/coincidence tolerance
(epsilon), defaulting to `1.0e-8`, used to decide whether points are treated as
coplanar or coincident during PLC processing, mirroring TetGen's `-T#` switch.
A larger tolerance SHALL make the mesher more willing to treat near-coplanar facets
as coplanar. (oracle: tetgen.cxx:3576–3589; tetgen.h:687)

#### Scenario: Default tolerance
- WHEN no tolerance is set
- THEN the coplanarity epsilon is `1.0e-8`

#### Scenario: Loosened tolerance
- WHEN the tolerance is set to `1e-6`
- THEN points within the larger tolerance are treated as coplanar during PLC processing

### Requirement: Diagnostic exact-arithmetic control

CyberMeshGenerator SHALL provide a typed predicate mode selecting exact-adaptive
evaluation (default), floating-point-only evaluation (disabling exact escalation),
or filter-disabled evaluation, mirroring TetGen's `-X` / `-X1`. The non-exact modes
are diagnostic and MAY produce incorrect results on degenerate input; the default
SHALL be exact-adaptive. (oracle: tetgen.cxx:3404–3410; tetgen.h:640–641)

#### Scenario: Default is exact
- WHEN `MeshOptions` is left at defaults
- THEN predicates use exact-adaptive evaluation

#### Scenario: Diagnostic floating-point mode
- WHEN the predicate mode is set to floating-point-only
- THEN predicates use the floating-point estimate without exact escalation, trading
  robustness for a diagnostic comparison

### Requirement: Batched predicate interface for acceleration

CyberMeshGenerator SHALL expose a batched predicate form (e.g. `orient3d_batch`,
`insphere_batch`) that evaluates a predicate over many candidate simplices at once,
returning a sign per candidate. The batched fast-filter stage SHALL be eligible for
GPU offload via the backend-acceleration substrate, while candidates whose sign is
uncertain SHALL fall back to exact CPU escalation, so the batched result is
identical to evaluating each candidate individually.

#### Scenario: Batched result equals per-item evaluation
- GIVEN a batch of candidate simplices
- WHEN `orient3d_batch` is evaluated
- THEN each returned sign equals the sign that scalar `orient3d` would return for
  that candidate

#### Scenario: Uncertain candidates escalate exactly
- GIVEN a batch where some candidates are near-degenerate
- WHEN the batch is evaluated with the device fast-filter enabled
- THEN the near-degenerate candidates are resolved by exact CPU escalation and their
  signs are correct
