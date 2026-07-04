# Tasks — Command-line interface (Phase 11)
- [x] `cmg::cli::run(args, out, err)` (include/cmg/cli + src/cli/cli.cpp): switch/file parse, read by extension, mesh, write `.1.node/.1.ele/.1.face`
- [x] Usage/help + error paths (unreadable input, mesh failure) — no crash
- [x] Thin `cli/main.cpp` executable target `cmg` behind `CMG_BUILD_CLI`
- [x] Wire into CMake
- [x] Tests: run on a temp .node -> outputs exist + re-readable; help returns 0; missing input returns non-zero
- [x] `openspec validate --all --strict` green; suite passes across profiles

## Deferred
- [ ] Full TetGen switch coverage; auxiliary outputs (-v/-g/-k); companion input files
