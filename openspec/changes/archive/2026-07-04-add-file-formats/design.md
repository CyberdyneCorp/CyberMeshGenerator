# Design — mesh file formats

## Layer shape

A standalone `cmg::io` layer, one translation unit per format, over the core
types. Nothing in the meshing kernels depends on it; it depends only on `Point3`,
`PLC`, `Mesh`, and the error/result model.

```
include/cmg/io/
  io.hpp        # public dispatch API + Format enum + WriteResult
  formats.hpp   # per-format read_/write_ declarations (the contract)
  detail.hpp    # inline shared parse helpers (comment strip, tokenize, node section)
src/io/
  dispatch.cpp  # detect() + read_plc/read_points/read_mesh/write_mesh/write_plc
  node.cpp ele.cpp faceedge.cpp poly.cpp
  stl.cpp off.cpp ply.cpp vtk.cpp medit.cpp
tests/io/       # one round-trip test per format
```

## Public API (extension-dispatched)

```cpp
namespace cmg::io {
enum class Format { Auto, Node, Poly, Smesh, Ele, Face, Edge, Neigh,
                    Stl, Off, Ply, Vtk, Medit };
using WriteResult = expected<std::monostate, MeshError>;   // void-free success

Format detect(std::string_view path);
expected<std::vector<Point3>, MeshError> read_points(const std::string& path);
expected<PLC,  MeshError> read_plc (const std::string& path);
expected<Mesh, MeshError> read_mesh(const std::string& path); // .ele+companion, vtk, medit
WriteResult write_mesh(const std::string& path, const Mesh&, Format = Format::Auto);
WriteResult write_plc (const std::string& path, const PLC&,  Format = Format::Auto);
}
```

`WriteResult` uses `std::monostate` as the success type because the project's
`cmg::expected` fallback (C++20) cannot hold `void`.

## Shared conventions (`detail.hpp`)

All TetGen ASCII files share: `#` starts a comment to end-of-line; fields are
whitespace- or comma-separated; objects are numbered from 0 or 1 with the base
auto-detected from the first data record. `detail.hpp` centralizes this so every
parser behaves identically:

- `next_data_line(stream)` — returns the next non-blank line with any `#…` comment
  stripped.
- `tokenize(line)` — splits on whitespace/commas.
- `read_node_section(...)` — parses the `<#pts> <dim> <#attrs> <marker>` header +
  point records (shared by `.node`, `.poly`, `.smesh`); records the detected index
  base so downstream indices are normalized to 0-based internally.
- Numeric parse helpers returning errors (not exceptions) on malformed input.

Internally the library is **0-based**; the detected base is subtracted on read and
`index_base` controls the base written back out.

## Format contract (`formats.hpp`)

Each format declares concrete free functions the dispatcher calls, e.g.
`expected<PLC,MeshError> read_off(const std::string&)`,
`WriteResult write_off(const std::string&, const PLC&)`. Declaring them centrally
lets the dispatcher and the format TUs agree without coupling the format
implementations to each other (the one exception: `.poly`/`.smesh` reuse
`read_node_section`/`write_node_section`, which live in `detail.hpp`/`node.cpp`).

## Mapping formats to core types

- **Surface/boundary formats** (`.poly`, `.smesh`, STL, OFF, PLY, and the surface
  side of VTK/Medit) → `PLC`: vertices into `PLC::points`, each polygon/triangle
  into a `Facet` with one `Polygon`; facet markers preserved.
- **Volumetric formats** (`.ele`+`.node`, volumetric VTK/Medit) → `Mesh`:
  tetrahedra, faces, markers, and (for `.neigh`) neighbor arrays.
- **STL vertex merge**: STL repeats coordinates per triangle; on read, coincident
  vertices are merged (hashed on rounded coordinates) so the PLC has shared
  vertices, matching TetGen's behavior.

## Errors

Malformed headers, out-of-range indices, truncated records, and unreadable files
return a `MeshError` (`InvalidInput`), never an exception across the API. Writers
report I/O failures the same way.

## Testing

Each format has a **round-trip** test: build a small `Mesh`/`PLC`, write it, read
it back, and assert points/connectivity/markers match. Readers are also tested on
a hand-written literal sample (comments, both index bases). The full-suite oracle
test — write `.node`/`.poly`, invoke real TetGen, read its `.ele`/`.face`, diff —
is added on top once the native readers/writers land.

## Parallel implementation

The formats are independent, so implementation fans out (one agent per format
module) against this fixed interface: each writes its own `src/io/<fmt>.cpp` +
`tests/io/test_<fmt>.cpp` and compiles its own TU. The dispatcher and the CMake/
test wiring are integrated centrally afterward.
