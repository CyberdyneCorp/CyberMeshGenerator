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

    cmg_mesh_destroy(mesh);
    cmg_options_destroy(opts);
    cmg_options_destroy(bad);
    printf(ok ? "C ABI smoke: OK\n" : "C ABI smoke: FAIL\n");
    return ok ? 0 : 1;
}
