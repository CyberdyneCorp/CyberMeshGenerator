// CyberMeshGenerator — stable C ABI shim implementation.
//
// Translates the C ABI onto the C++ core. All C++ exceptions are caught at the
// boundary and converted to status codes; nothing throws across `extern "C"`.
#include "cmg_c/cmg_c.h"

#include <cstring>
#include <exception>
#include <string>
#include <vector>

#include "cmg/cmg.hpp"

// Opaque handle definitions.
struct cmg_plc { cmg::PLC value; };
struct cmg_options { cmg::MeshOptions value; };
struct cmg_mesh {
    cmg::Mesh value;
    // Flattened, ABI-stable views computed once on creation.
    std::vector<double> points;
    std::vector<int> tets;
    std::vector<int> faces;
};

namespace {

void set_err(char* buf, size_t len, const std::string& msg) {
    if (buf && len) {
        std::strncpy(buf, msg.c_str(), len - 1);
        buf[len - 1] = '\0';
    }
}

cmg_status to_status(cmg::MeshErrorCode c) {
    using E = cmg::MeshErrorCode;
    switch (c) {
        case E::InvalidInput:      return CMG_ERR_INVALID_INPUT;
        case E::SelfIntersection:  return CMG_ERR_SELF_INTERSECTION;
        case E::UnsupportedOption: return CMG_ERR_UNSUPPORTED_OPTION;
        case E::QualityNotMet:     return CMG_ERR_QUALITY_NOT_MET;
        case E::NotImplemented:    return CMG_ERR_NOT_IMPLEMENTED;
        case E::Internal:          return CMG_ERR_INTERNAL;
    }
    return CMG_ERR_INTERNAL;
}

cmg_mesh* flatten(cmg::Mesh&& m) {
    auto* h = new cmg_mesh{std::move(m), {}, {}, {}};
    h->points.reserve(h->value.points.size() * 3);
    for (const auto& p : h->value.points) {
        h->points.push_back(static_cast<double>(p.x));
        h->points.push_back(static_cast<double>(p.y));
        h->points.push_back(static_cast<double>(p.z));
    }
    for (const auto& t : h->value.tetrahedra)
        h->tets.insert(h->tets.end(), t.begin(), t.end());
    for (const auto& f : h->value.faces)
        h->faces.insert(h->faces.end(), f.begin(), f.end());
    return h;
}

} // namespace

extern "C" {

cmg_plc* cmg_plc_create(void) { return new cmg_plc{}; }
void cmg_plc_destroy(cmg_plc* p) { delete p; }
cmg_options* cmg_options_create(void) { return new cmg_options{}; }
void cmg_options_destroy(cmg_options* o) { delete o; }
void cmg_mesh_destroy(cmg_mesh* m) { delete m; }

cmg_status cmg_plc_set_points(cmg_plc* plc, const double* xyz, size_t count) {
    if (!plc || (!xyz && count)) return CMG_ERR_INVALID_INPUT;
    plc->value.points.clear();
    plc->value.points.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        plc->value.points.push_back(
            {static_cast<cmg::Real>(xyz[3 * i]),
             static_cast<cmg::Real>(xyz[3 * i + 1]),
             static_cast<cmg::Real>(xyz[3 * i + 2])});
    }
    return CMG_OK;
}

cmg_status cmg_options_from_switches(cmg_options* o, const char* sw,
                                     char* errbuf, size_t errbuf_len) {
    if (!o || !sw) return CMG_ERR_INVALID_INPUT;
    auto r = cmg::MeshOptions::from_switches(sw);
    if (!r) {
        set_err(errbuf, errbuf_len, r.error().message);
        return CMG_ERR_PARSE;
    }
    o->value = *r;
    return CMG_OK;
}

cmg_status cmg_tetrahedralize(const cmg_plc* plc, const cmg_options* opts,
                              cmg_mesh** out, char* errbuf, size_t errbuf_len) {
    if (out) *out = nullptr;
    if (!plc || !opts || !out) return CMG_ERR_INVALID_INPUT;
    try {
        auto r = cmg::tetrahedralize(plc->value, opts->value);
        if (!r) {
            set_err(errbuf, errbuf_len, r.error().message);
            return to_status(r.error().code);
        }
        *out = flatten(std::move(*r));
        return CMG_OK;
    } catch (const std::exception& e) {
        set_err(errbuf, errbuf_len, e.what());
        return CMG_ERR_INTERNAL;
    }
}

cmg_status cmg_delaunay(const double* xyz, size_t count, const cmg_options* opts,
                        cmg_mesh** out, char* errbuf, size_t errbuf_len) {
    if (out) *out = nullptr;
    if ((!xyz && count) || !opts || !out) return CMG_ERR_INVALID_INPUT;
    try {
        std::vector<cmg::Point3> pts;
        pts.reserve(count);
        for (size_t i = 0; i < count; ++i)
            pts.push_back({static_cast<cmg::Real>(xyz[3 * i]),
                           static_cast<cmg::Real>(xyz[3 * i + 1]),
                           static_cast<cmg::Real>(xyz[3 * i + 2])});
        auto r = cmg::delaunay(pts, opts->value);
        if (!r) {
            set_err(errbuf, errbuf_len, r.error().message);
            return to_status(r.error().code);
        }
        *out = flatten(std::move(*r));
        return CMG_OK;
    } catch (const std::exception& e) {
        set_err(errbuf, errbuf_len, e.what());
        return CMG_ERR_INTERNAL;
    }
}

size_t cmg_mesh_num_points(const cmg_mesh* m) { return m ? m->value.point_count() : 0; }
size_t cmg_mesh_num_tets(const cmg_mesh* m) { return m ? m->value.tet_count() : 0; }
size_t cmg_mesh_num_faces(const cmg_mesh* m) { return m ? m->value.face_count() : 0; }
const double* cmg_mesh_points(const cmg_mesh* m) { return m ? m->points.data() : nullptr; }
const int* cmg_mesh_tets(const cmg_mesh* m) { return m ? m->tets.data() : nullptr; }
const int* cmg_mesh_faces(const cmg_mesh* m) { return m ? m->faces.data() : nullptr; }

const char* cmg_version(void) { return CMG_VERSION_STRING; }

} // extern "C"
