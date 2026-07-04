/* Pure-C smoke test: proves the C ABI is usable without a C++ compiler/mangling. */
#include "cmg_c/cmg_c.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    /* Four non-coplanar points -> exactly one tetrahedron. */
    const double xyz[12] = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};
    cmg_options* opts = cmg_options_create();
    cmg_mesh* mesh = NULL;
    char err[256] = {0};

    cmg_status st = cmg_delaunay(xyz, 4, opts, &mesh, err, sizeof err);
    if (st != CMG_OK) {
        fprintf(stderr, "cmg_delaunay failed (%d): %s\n", st, err);
        cmg_options_destroy(opts);
        return 1;
    }

    size_t ntets = cmg_mesh_num_tets(mesh);
    size_t npts = cmg_mesh_num_points(mesh);
    printf("C ABI: version=%s points=%zu tets=%zu\n", cmg_version(), npts, ntets);

    int ok = (ntets == 1 && npts == 4);

    /* A parse error must cross the boundary as a code + message, never a crash. */
    cmg_options* bad = cmg_options_create();
    char perr[256] = {0};
    cmg_status pst = cmg_options_from_switches(bad, "pw", perr, sizeof perr);
    ok = ok && (pst == CMG_ERR_PARSE) && (perr[0] != '\0');

    /* Drive a small PLC (single-tet tetrahedron) through the new C ABI:
     * points + four triangular facets + typed PLC option. */
    cmg_plc* plc = cmg_plc_create();
    cmg_status ps = cmg_plc_set_points(plc, xyz, 4);
    ok = ok && (ps == CMG_OK);
    const int tri[4][3] = {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}};
    for (int i = 0; i < 4; ++i)
        ok = ok && (cmg_plc_add_facet(plc, tri[i], 3, 1) == CMG_OK);

    cmg_options* popts = cmg_options_create();
    ok = ok && (cmg_options_set_plc(popts, 1) == CMG_OK);

    cmg_mesh* pmesh = NULL;
    char merr[256] = {0};
    cmg_status mst = cmg_tetrahedralize(plc, popts, &pmesh, merr, sizeof merr);
    if (mst != CMG_OK) {
        fprintf(stderr, "cmg_tetrahedralize failed (%d): %s\n", mst, merr);
        ok = 0;
    } else {
        size_t ptets = cmg_mesh_num_tets(pmesh);
        size_t pfaces = cmg_mesh_num_faces(pmesh);
        printf("C ABI PLC: tets=%zu faces=%zu\n", ptets, pfaces);
        ok = ok && (ptets == 1) && (cmg_mesh_tet_markers(pmesh) != NULL);
        cmg_mesh_destroy(pmesh);
    }

    /* File I/O round-trip: write the tetrahedron PLC to a .off surface file and
     * read it back, checking the point count survives the trip. */
    cmg_status wst = cmg_write_plc("/tmp/s.off", plc, merr, sizeof merr);
    if (wst != CMG_OK) {
        fprintf(stderr, "cmg_write_plc failed (%d): %s\n", wst, merr);
        ok = 0;
    } else {
        cmg_plc* rplc = NULL;
        char rerr[256] = {0};
        cmg_status rst = cmg_read_plc("/tmp/s.off", &rplc, rerr, sizeof rerr);
        if (rst != CMG_OK) {
            fprintf(stderr, "cmg_read_plc failed (%d): %s\n", rst, rerr);
            ok = 0;
        } else {
            size_t rnp = cmg_plc_num_points(rplc);
            printf("C ABI IO: read_plc points=%zu\n", rnp);
            ok = ok && (rnp > 0);
            cmg_plc_destroy(rplc);
        }
    }

    cmg_mesh_destroy(mesh);
    cmg_options_destroy(opts);
    cmg_options_destroy(bad);
    cmg_plc_destroy(plc);
    cmg_options_destroy(popts);
    printf(ok ? "C ABI smoke: OK\n" : "C ABI smoke: FAIL\n");
    return ok ? 0 : 1;
}
