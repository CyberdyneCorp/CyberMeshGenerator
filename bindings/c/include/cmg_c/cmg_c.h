/* CyberMeshGenerator — stable C ABI shim.
 *
 * Both the Python and Swift bindings sit on this C interface rather than on the
 * C++ types directly, so they do not depend on C++ name mangling or ABI and can
 * version independently. Failures cross the boundary as return codes plus an
 * out-error-message buffer; no C++ exception ever escapes.
 */
#ifndef CMG_C_H
#define CMG_C_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Result codes mirror cmg::MeshErrorCode plus an OK. */
typedef enum cmg_status {
    CMG_OK = 0,
    CMG_ERR_INVALID_INPUT = 1,
    CMG_ERR_SELF_INTERSECTION = 2,
    CMG_ERR_UNSUPPORTED_OPTION = 3,
    CMG_ERR_QUALITY_NOT_MET = 4,
    CMG_ERR_NOT_IMPLEMENTED = 5,
    CMG_ERR_INTERNAL = 6,
    CMG_ERR_PARSE = 7
} cmg_status;

/* Opaque handles — the C++ objects are never exposed by layout. */
typedef struct cmg_plc cmg_plc;
typedef struct cmg_mesh cmg_mesh;
typedef struct cmg_options cmg_options;

/* --- lifecycle ---------------------------------------------------------- */
cmg_plc* cmg_plc_create(void);
void cmg_plc_destroy(cmg_plc*);
cmg_options* cmg_options_create(void);
void cmg_options_destroy(cmg_options*);
void cmg_mesh_destroy(cmg_mesh*);

/* --- input -------------------------------------------------------------- */
/* Set the PLC point cloud from a flat [x0,y0,z0, x1,y1,z1, ...] array. */
cmg_status cmg_plc_set_points(cmg_plc*, const double* xyz, size_t count);

/* Append a single-polygon facet whose boundary loop is the `count` vertex
 * indices in `poly_vertex_indices` (a triangle when count==3, any polygon
 * otherwise). The indices refer to points previously set via
 * cmg_plc_set_points. `marker` is the boundary marker propagated to output
 * faces. */
cmg_status cmg_plc_add_facet(cmg_plc*, const int* poly_vertex_indices,
                             size_t count, int marker);

/* Append a volumetric hole seed point (interior of material to remove). */
cmg_status cmg_plc_add_hole(cmg_plc*, double x, double y, double z);

/* Append a material region seed at (x,y,z) with the given attribute and
 * per-region maximum tetrahedron volume (negative => unconstrained). */
cmg_status cmg_plc_add_region(cmg_plc*, double x, double y, double z,
                              double attribute, double max_volume);

/* Parse a TetGen-style switch string (without leading dash) into options. On
 * failure returns CMG_ERR_PARSE and fills errbuf. */
cmg_status cmg_options_from_switches(cmg_options*, const char* switches,
                                     char* errbuf, size_t errbuf_len);

/* --- typed option setters ----------------------------------------------- */
/* Enable/disable PLC (boundary-conforming) meshing (-p). */
cmg_status cmg_options_set_plc(cmg_options*, int on);
/* Set the global maximum tetrahedron volume (-a#). */
cmg_status cmg_options_set_max_volume(cmg_options*, double max_volume);
/* Enable quality refinement (-q) with a maximum radius-edge ratio and a minimum
 * dihedral angle (degrees). */
cmg_status cmg_options_set_quality(cmg_options*, double radius_edge,
                                   double min_dihedral);
/* Recover PLC facet edges as mesh-edge chains. */
cmg_status cmg_options_set_preserve_edges(cmg_options*, int on);

/* --- meshing ------------------------------------------------------------ */
/* Tetrahedralize the PLC. On CMG_OK, *out receives a mesh handle the caller must
 * free with cmg_mesh_destroy. On failure fills errbuf and leaves *out NULL. */
cmg_status cmg_tetrahedralize(const cmg_plc*, const cmg_options*,
                              cmg_mesh** out, char* errbuf, size_t errbuf_len);

/* Delaunay of a flat point array. */
cmg_status cmg_delaunay(const double* xyz, size_t count, const cmg_options*,
                        cmg_mesh** out, char* errbuf, size_t errbuf_len);

/* --- output (zero-copy views into the mesh handle's storage) ------------ */
size_t cmg_mesh_num_points(const cmg_mesh*);
size_t cmg_mesh_num_tets(const cmg_mesh*);
size_t cmg_mesh_num_faces(const cmg_mesh*);
/* Points as [x,y,z,...] (3*num_points doubles); tets/faces as flat int indices
 * (4*num_tets / 3*num_faces). Pointers are valid until the mesh is destroyed. */
const double* cmg_mesh_points(const cmg_mesh*);
const int* cmg_mesh_tets(const cmg_mesh*);
const int* cmg_mesh_faces(const cmg_mesh*);
/* Per-tetrahedron region/material markers (num_tets ints) and per-face boundary
 * markers (num_faces ints). Either may be NULL if the mesh carries none. */
const int* cmg_mesh_tet_markers(const cmg_mesh*);
const int* cmg_mesh_face_markers(const cmg_mesh*);

/* Library version string, e.g. "0.1.0". */
const char* cmg_version(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CMG_C_H */
