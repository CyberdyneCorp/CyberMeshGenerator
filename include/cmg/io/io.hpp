// CyberMeshGenerator — file I/O: public, extension-dispatched API.
//
// A standalone serialization layer over the core Point3 / PLC / Mesh types. Reads
// and writes TetGen's native ASCII containers and the common interchange formats.
// Everything returns cmg::expected<…, MeshError>; nothing throws across the API.
#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::io {

/// Supported file formats. `Auto` selects by extension.
enum class Format {
    Auto,
    Node,  ///< .node  — point list
    Poly,  ///< .poly  — PLC
    Smesh, ///< .smesh — simplified PLC
    Ele,   ///< .ele   — tetrahedra
    Face,  ///< .face  — triangular faces
    Edge,  ///< .edge  — edges
    Neigh, ///< .neigh — tetrahedron adjacency
    Stl,   ///< .stl   — stereolithography surface
    Off,   ///< .off   — Geomview surface
    Ply,   ///< .ply   — Stanford PLY surface (ASCII)
    Vtk,   ///< .vtk   — legacy VTK
    Medit, ///< .mesh  — Medit
    Obj,   ///< .obj   — Wavefront OBJ surface
};

/// Success type for writers (cmg::expected cannot hold void on C++20).
using WriteResult = expected<std::monostate, MeshError>;
inline WriteResult write_ok() { return WriteResult{std::monostate{}}; }

/// Detect the format from a path's extension (Format::Auto if unknown).
Format detect(std::string_view path);

/// Read a point set (`.node`).
expected<std::vector<Point3>, MeshError> read_points(const std::string& path);

/// Read a PLC boundary (`.poly`/`.smesh`/`.stl`/`.off`/`.ply`/`.vtk`/`.mesh`).
expected<PLC, MeshError> read_plc(const std::string& path);

/// Read a volumetric mesh (`.ele` with companion `.node`, or volumetric VTK/Medit).
expected<Mesh, MeshError> read_mesh(const std::string& path);

/// Write a mesh; format inferred from the extension unless given explicitly.
WriteResult write_mesh(const std::string& path, const Mesh& mesh,
                       Format format = Format::Auto);

/// Write a PLC; format inferred from the extension unless given explicitly.
WriteResult write_plc(const std::string& path, const PLC& plc,
                      Format format = Format::Auto);

} // namespace cmg::io
