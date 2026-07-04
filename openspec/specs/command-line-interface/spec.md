# command-line-interface Specification

## Purpose
TBD - created by archiving change add-command-line-interface. Update Purpose after archive.
## Requirements
### Requirement: TetGen-style command invocation

CyberMeshGenerator SHALL provide `cmg::cli::run(args, out, err)` that parses
leading-dash arguments as TetGen switches and the last non-switch argument as the
input file, reads that input by extension, tetrahedralizes it, and writes
`<base>.1.node`, `<base>.1.ele`, and `<base>.1.face`, returning 0 on success and
non-zero on failure. (oracle: TetGen command-line-interface; manual §4.1)

#### Scenario: Mesh a point set from the shell
- GIVEN a `.node` file of points and args `{"cloud.node"}`
- WHEN `cli::run` is invoked
- THEN it writes `cloud.1.node` and `cloud.1.ele` describing the Delaunay mesh and
  returns 0

#### Scenario: PLC with switches
- GIVEN args `{"-pq1.414a0.1", "part.smesh"}`
- WHEN `cli::run` is invoked
- THEN the PLC is meshed with those options and the `.1.*` outputs are written

### Requirement: Usage and error handling

CyberMeshGenerator's CLI SHALL print usage and return 0 when given `-h`, `-?`, or no
input file, and SHALL return non-zero (with a message to `err`) when the input file
cannot be read or meshing fails, without crashing. (oracle: TetGen usage/help)

#### Scenario: Help
- WHEN `cli::run({"-h"}, ...)` is invoked
- THEN usage text is written and 0 is returned

#### Scenario: Missing input reported
- WHEN the input file does not exist
- THEN a non-zero code is returned and an error message is written to `err`

