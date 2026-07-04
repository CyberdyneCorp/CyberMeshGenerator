// CyberMeshGenerator — live TetGen oracle (frozen mode).
//
// Validates BOTH the file-format readers (against real TetGen 1.6.0 output) and
// the Delaunay kernel (against TetGen's topology). The golden files under
// tests/oracle/golden/ were produced by running real TetGen 1.6.0 on cloud.node;
// CI reads them without needing a TetGen build. For a general-position point set
// the Delaunay tetrahedralization is unique, so the set of tetrahedra (as sorted
// vertex-index tuples) MUST match TetGen exactly.
#include "cmg/cmg.hpp"
#include "cmg/io/io.hpp"
#include "harness.hpp"

#include <array>
#include <set>
#include <string>

using namespace cmg;

namespace {

std::set<std::array<int, 4>> tuple_set(const Mesh& m) {
    std::set<std::array<int, 4>> s;
    for (const auto& t : m.tetrahedra) {
        std::array<int, 4> a{t[0], t[1], t[2], t[3]};
        std::sort(a.begin(), a.end());
        s.insert(a);
    }
    return s;
}

std::string golden(const char* name) {
    return std::string(CMG_GOLDEN_DIR) + "/" + name;
}

} // namespace

CMG_TEST("oracle: read TetGen's .node input via cmg::io") {
    auto pts = io::read_points(golden("cloud.node"));
    CMG_CHECK(bool(pts));
    CMG_CHECK(pts->size() == 24);
}

CMG_TEST("oracle: read TetGen's .1.ele mesh output via cmg::io") {
    auto tg = io::read_mesh(golden("cloud.1.ele"));
    CMG_CHECK(bool(tg));
    CMG_CHECK(tg->tet_count() == 78); // TetGen reported 78 tetrahedra
}

CMG_TEST("oracle: our Delaunay matches TetGen's tetrahedralization exactly") {
    auto pts = io::read_points(golden("cloud.node"));
    CMG_CHECK(bool(pts));

    auto ours = delaunay(*pts, {});
    CMG_CHECK(bool(ours));

    auto tg = io::read_mesh(golden("cloud.1.ele"));
    CMG_CHECK(bool(tg));

    // General position => unique DT => identical sorted-tuple sets.
    CMG_CHECK(ours->tet_count() == tg->tet_count());
    CMG_CHECK(tuple_set(*ours) == tuple_set(*tg));
}
