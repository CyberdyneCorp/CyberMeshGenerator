// CyberMeshGenerator — tests for the command-line driver (cmg::cli::run).
#include "cmg/cli/cli.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "cmg/io/io.hpp"
#include "harness.hpp"

namespace {

// Write a `.node` file with the 8 corners of the unit cube.
void write_cube_node(const std::string& path) {
    std::ofstream f(path);
    f << "8 3 0 0\n";
    f << "1 0 0 0\n";
    f << "2 1 0 0\n";
    f << "3 1 1 0\n";
    f << "4 0 1 0\n";
    f << "5 0 0 1\n";
    f << "6 1 0 1\n";
    f << "7 1 1 1\n";
    f << "8 0 1 1\n";
}

bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

} // namespace

CMG_TEST("cli meshes a .node point cloud and writes re-readable outputs") {
    const std::string in = "/tmp/cmg_cli_in.node";
    write_cube_node(in);

    std::ostringstream oss, ess;
    int rc = cmg::cli::run({in}, oss, ess);
    CMG_CHECK(rc == 0);

    CMG_CHECK(file_exists("/tmp/cmg_cli_in.1.ele"));

    auto mesh = cmg::io::read_mesh("/tmp/cmg_cli_in.1.ele");
    CMG_CHECK(mesh.has_value());
    CMG_CHECK(mesh->tet_count() > 0);
}

CMG_TEST("cli prints usage and returns 0 for -h") {
    std::ostringstream oss, ess;
    int rc = cmg::cli::run({"-h"}, oss, ess);
    CMG_CHECK(rc == 0);
    CMG_CHECK(!oss.str().empty());
}

CMG_TEST("cli reports a nonzero code for a missing input file") {
    std::ostringstream oss, ess;
    int rc = cmg::cli::run({"/tmp/does_not_exist.node"}, oss, ess);
    CMG_CHECK(rc != 0);
}
