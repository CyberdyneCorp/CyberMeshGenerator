// CyberMeshGenerator — Voronoi diagram file output (TetGen .v.* format).
#pragma once

#include <string>

#include "cmg/io/io.hpp"
#include "cmg/voronoi/voronoi.hpp"

namespace cmg::io {

/// Write the Voronoi diagram to TetGen's `.v.node` (vertices) and `.v.edge`
/// (edges, rays with `v2 = -1` and a direction) files, derived from `base_path`
/// (its extension is replaced). Also writes `.v.cell` (per-site incident vertex
/// lists). Objects are numbered from `index_base`. (TetGen -v output; manual §5.2.10)
WriteResult write_voronoi(const std::string& base_path,
                          const voronoi::VoronoiDiagram& diagram,
                          int index_base = 0);

} // namespace cmg::io
