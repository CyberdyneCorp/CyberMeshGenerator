# Add command-line interface (Phase 11)

## Why

The library has a typed API and a TetGen-compatible switch parser, but no runnable
`tetgen`-style executable. A CLI makes the tool usable from the shell and directly
comparable to TetGen (same switches, same output-file naming), completing the
roadmap's user-facing surface.

## What changes

Spec delta for the **command-line-interface** capability:

- `cmg::cli::run(args, out, err)` — parses TetGen switches (leading-dash args via
  `MeshOptions::from_switches`) and the input file (last non-switch arg), reads the
  input by extension (`cmg::io`), tetrahedralizes, and writes `<base>.1.node`,
  `<base>.1.ele`, `<base>.1.face`. Returns 0 on success. `-h`/`-?`/no-input prints
  usage. Logic lives in the library (testable); a thin `cli/main.cpp` executable
  (`cmg` / `CMG_BUILD_CLI`) calls it.

## Impact

- `cmg -pq1.414a0.1 part.poly` meshes a PLC and writes TetGen-named outputs;
  `cmg cloud.node` writes the Delaunay mesh — directly diffable against TetGen.

## Non-goals
- **Full TetGen switch coverage** — the subset the typed options already model
  (`-p/-q/-a/-z/-Y/...`); unmodelled switches are ignored as before.
- **Voronoi (`-v`) / Medit (`-g`) / VTK (`-k`) auxiliary outputs** from the CLI —
  the writers exist; wiring every output flag is deferred.
- **Reading additional-points / background-mesh companion files** from the CLI.
