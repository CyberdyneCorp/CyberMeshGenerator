# language-bindings Specification (surface-simplification delta)

## ADDED Requirements

### Requirement: Surface simplification exposed through the bindings

The C ABI SHALL expose `cmg_plc_simplify` (returning a new PLC handle), and the Python
and Swift bindings SHALL expose `simplify(plc, grid)` → `PLC` over it, so a loaded PLC
can be decimated before meshing without re-implementing vertex clustering in the
binding. Failures SHALL cross the C boundary as status codes plus a message, never as a
thrown exception. (oracle: language-bindings)

#### Scenario: Simplify a loaded PLC from Python
- GIVEN a PLC loaded from a dense surface file
- WHEN `cybermesh.simplify(plc, grid=34)` is called
- THEN a new `PLC` with fewer triangles is returned, and it can be passed to
  `cybermesh.tetrahedralize`

#### Scenario: Invalid grid is a clean error
- WHEN `cmg_plc_simplify` is given a grid less than 1
- THEN a non-zero status and an error message are returned and `*out` is left NULL
