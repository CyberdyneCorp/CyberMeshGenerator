// CyberMeshGenerator — `cmg` command-line executable (thin wrapper over cmg::cli).
#include <iostream>
#include <string>
#include <vector>

#include "cmg/cli/cli.hpp"

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    return cmg::cli::run(args, std::cout, std::cerr);
}
