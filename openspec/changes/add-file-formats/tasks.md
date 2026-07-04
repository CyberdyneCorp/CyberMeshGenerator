# Tasks — Add mesh file formats (Phase 2)

## Shared interface (integrated centrally)
- [x] `io.hpp` (Format enum, WriteResult, dispatch decls), `formats.hpp` (per-format contract)
- [x] `detail.hpp` shared helpers: comment strip, tokenize, numeric parse, node-section read/write, index-base handling
- [x] `dispatch.cpp`: `detect()` + `read_points`/`read_plc`/`read_mesh`/`write_mesh`/`write_plc` extension routing
- [x] CMake: compile `src/io/*.cpp` into `cmg`; register `tests/io/*` in the suite

## Format modules (parallel — one per module)
- [x] `.node` read/write (+ shared node-section helper)
- [x] `.ele` read/write
- [x] `.face` / `.edge` / `.neigh` read/write
- [x] `.poly` / `.smesh` PLC read/write (facets, holes, regions)
- [x] STL read/write (ASCII + binary, coincident-vertex merge)
- [x] OFF read/write
- [x] PLY (ASCII) read/write
- [x] legacy VTK read/write
- [x] Medit `.mesh` read/write

## Tests & oracle
- [x] Round-trip test per format (write → read → compare points/connectivity/markers)
- [x] Reader tests on literal samples (comments, both index bases)
- [x] Unknown-extension and malformed-input error paths
- [x] Live TetGen oracle: write `.node`/`.poly`, run TetGen, read `.ele`/`.face`, diff topology
- [x] `openspec validate --all --strict` green; suite passes CPU-only across profiles
