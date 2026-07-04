// CyberMeshGenerator — TetGen-compatible command-line interface.
#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace cmg::cli {

/// Run the CLI: `args` are the command-line arguments after the program name,
/// e.g. {"-pq1.414a0.1", "part.poly"} or {"cloud.node"}. Leading-dash switches are
/// parsed as TetGen switches (via MeshOptions::from_switches); the last non-switch
/// argument is the input file. Reads the input by extension, tetrahedralizes, and
/// writes `<base>.1.node` / `<base>.1.ele` / `<base>.1.face`. Diagnostics go to
/// `out`, errors to `err`. Returns 0 on success, non-zero on failure. With no
/// input file (or `-h`/`-?`) prints usage and returns 0.
int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err);

} // namespace cmg::cli
