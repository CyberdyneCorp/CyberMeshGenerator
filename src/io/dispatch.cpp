// CyberMeshGenerator — file I/O extension dispatch.
//
// Routes the public read_*/write_* entry points to the concrete per-format
// implementations (declared in formats.hpp) based on the file extension or an
// explicit Format. The per-format translation units are independent; only this
// file knows the whole set.
#include "cmg/io/io.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "cmg/io/formats.hpp"

namespace cmg::io {

namespace {

std::string lower_ext(std::string_view path) {
    auto dot = path.rfind('.');
    if (dot == std::string_view::npos) return {};
    std::string ext(path.substr(dot + 1));
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

std::string replace_ext(const std::string& path, const char* ext) {
    auto dot = path.rfind('.');
    return (dot == std::string::npos ? path : path.substr(0, dot)) + ext;
}

unexpected<MeshError> unsupported(const std::string& what) {
    return unexpected(MeshError{MeshErrorCode::InvalidInput,
                                "unsupported file format for " + what});
}

} // namespace

Format detect(std::string_view path) {
    std::string e = lower_ext(path);
    if (e == "node") return Format::Node;
    if (e == "poly") return Format::Poly;
    if (e == "smesh") return Format::Smesh;
    if (e == "ele") return Format::Ele;
    if (e == "face") return Format::Face;
    if (e == "edge") return Format::Edge;
    if (e == "neigh") return Format::Neigh;
    if (e == "stl") return Format::Stl;
    if (e == "off") return Format::Off;
    if (e == "ply") return Format::Ply;
    if (e == "vtk") return Format::Vtk;
    if (e == "mesh") return Format::Medit;
    if (e == "obj") return Format::Obj;
    return Format::Auto;
}

expected<std::vector<Point3>, MeshError> read_points(const std::string& path) {
    if (detect(path) == Format::Node) return read_node(path);
    return unsupported(path);
}

expected<PLC, MeshError> read_plc(const std::string& path) {
    switch (detect(path)) {
        case Format::Poly:  return read_poly(path);
        case Format::Smesh: return read_smesh(path);
        case Format::Stl:   return read_stl(path);
        case Format::Off:   return read_off(path);
        case Format::Ply:   return read_ply(path);
        case Format::Obj:   return read_obj(path);
        default:            return unsupported(path);
    }
}

expected<Mesh, MeshError> read_mesh(const std::string& path) {
    switch (detect(path)) {
        case Format::Ele: {
            auto pts = read_node(replace_ext(path, ".node"));
            if (!pts) return unexpected(pts.error());
            return read_ele(path, *pts);
        }
        case Format::Vtk:   return read_vtk(path);
        case Format::Medit: return read_medit(path);
        default:            return unsupported(path);
    }
}

WriteResult write_mesh(const std::string& path, const Mesh& mesh, Format format) {
    if (format == Format::Auto) format = detect(path);
    switch (format) {
        case Format::Ele: {
            // Emit the companion .node alongside the .ele (as TetGen does).
            auto n = write_node(replace_ext(path, ".node"), mesh.points,
                                mesh.point_markers,
                                mesh.index_base == IndexBase::One ? 1 : 0);
            if (!n) return n;
            return write_ele(path, mesh);
        }
        case Format::Face:  return write_face(path, mesh);
        case Format::Edge:  return write_edge(path, mesh);
        case Format::Neigh: return write_neigh(path, mesh);
        case Format::Vtk:   return write_vtk(path, mesh);
        case Format::Medit: return write_medit(path, mesh);
        case Format::Off:   return write_off_mesh(path, mesh);
        case Format::Stl:   return write_stl_mesh(path, mesh);
        case Format::Obj:   return write_obj_mesh(path, mesh);
        default:            return unsupported(path);
    }
}

WriteResult write_plc(const std::string& path, const PLC& plc, Format format) {
    if (format == Format::Auto) format = detect(path);
    switch (format) {
        case Format::Poly:  return write_poly(path, plc);
        case Format::Smesh: return write_smesh(path, plc);
        case Format::Off:   return write_off(path, plc);
        case Format::Ply:   return write_ply(path, plc);
        case Format::Stl:   return write_stl(path, plc);
        case Format::Obj:   return write_obj(path, plc);
        default:            return unsupported(path);
    }
}

} // namespace cmg::io
