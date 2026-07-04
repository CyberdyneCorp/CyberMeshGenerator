// CyberMeshGenerator — per-format reader/writer contract.
//
// Each format is implemented in its own translation unit (src/io/<fmt>.cpp) but
// declares its entry points here so the dispatcher (src/io/dispatch.cpp) can call
// them without the format implementations depending on one another.
#pragma once

#include <string>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/plc.hpp"
#include "cmg/io/io.hpp"

namespace cmg::io {

// --- TetGen native ---------------------------------------------------------
expected<std::vector<Point3>, MeshError> read_node(const std::string& path);
WriteResult write_node(const std::string& path, const std::vector<Point3>& pts,
                       const std::vector<int>& markers, int index_base);

expected<PLC, MeshError> read_poly(const std::string& path);   // .poly
expected<PLC, MeshError> read_smesh(const std::string& path);  // .smesh
WriteResult write_poly(const std::string& path, const PLC& plc);
WriteResult write_smesh(const std::string& path, const PLC& plc);

// .ele read needs the companion points (from the .node with the same base name).
expected<Mesh, MeshError> read_ele(const std::string& path,
                                   const std::vector<Point3>& points);
WriteResult write_ele(const std::string& path, const Mesh& mesh);

expected<std::vector<Triangle>, MeshError> read_face(const std::string& path);
WriteResult write_face(const std::string& path, const Mesh& mesh);
WriteResult write_edge(const std::string& path, const Mesh& mesh);
WriteResult write_neigh(const std::string& path, const Mesh& mesh);

// --- Interchange (surface -> PLC) -----------------------------------------
expected<PLC, MeshError> read_stl(const std::string& path);
WriteResult write_stl(const std::string& path, const PLC& plc); // ASCII
WriteResult write_stl_mesh(const std::string& path, const Mesh& mesh);

expected<PLC, MeshError> read_off(const std::string& path);
WriteResult write_off(const std::string& path, const PLC& plc);
WriteResult write_off_mesh(const std::string& path, const Mesh& mesh);

expected<PLC, MeshError> read_ply(const std::string& path);
WriteResult write_ply(const std::string& path, const PLC& plc);

expected<Mesh, MeshError> read_vtk(const std::string& path);
WriteResult write_vtk(const std::string& path, const Mesh& mesh);

expected<Mesh, MeshError> read_medit(const std::string& path);
WriteResult write_medit(const std::string& path, const Mesh& mesh);

expected<PLC, MeshError> read_obj(const std::string& path);       // Wavefront OBJ
WriteResult write_obj(const std::string& path, const PLC& plc);
WriteResult write_obj_mesh(const std::string& path, const Mesh& mesh);

// --- Constraint / sizing files --------------------------------------------
// .vol: a per-tetrahedron maximum volume (for -r refinement). Returns one value
// per tetrahedron; a zero or negative value means "unconstrained".
expected<std::vector<double>, MeshError> read_vol(const std::string& path,
                                                  std::size_t num_tets);
WriteResult write_vol(const std::string& path, const std::vector<double>& vols,
                      int index_base);

// .mtr: a per-node sizing metric (target edge length). Returns one value per node.
expected<std::vector<double>, MeshError> read_mtr(const std::string& path);
WriteResult write_mtr(const std::string& path,
                      const std::vector<double>& sizes);

} // namespace cmg::io
