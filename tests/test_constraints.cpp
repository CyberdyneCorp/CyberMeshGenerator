// CyberMeshGenerator — tests for .vol / .mtr constraint & sizing files.
#include <cmath>
#include <string>
#include <vector>

#include "cmg/io/formats.hpp"
#include "harness.hpp"

namespace {

std::string tmp(const std::string& name) {
    return "/tmp/cmg_constraints_" + name;
}

CMG_TEST(".vol round-trip preserves values") {
    const std::string path = tmp("roundtrip.vol");
    std::vector<double> vols = {0.5, 1.25, -1.0, 3.75};
    CMG_CHECK(cmg::io::write_vol(path, vols, 1).has_value());

    auto r = cmg::io::read_vol(path, vols.size());
    CMG_CHECK(r.has_value());
    CMG_CHECK(r->size() == vols.size());
    for (std::size_t i = 0; i < vols.size(); ++i)
        CMG_CHECK(std::fabs((*r)[i] - vols[i]) < 1e-9);
}

CMG_TEST(".vol count mismatch is an error") {
    const std::string path = tmp("mismatch.vol");
    std::vector<double> vols = {1.0, 2.0, 3.0};
    CMG_CHECK(cmg::io::write_vol(path, vols, 0).has_value());

    auto r = cmg::io::read_vol(path, 5); // wrong num_tets
    CMG_CHECK(!r.has_value());
}

CMG_TEST(".mtr round-trip preserves sizes") {
    const std::string path = tmp("roundtrip.mtr");
    std::vector<double> sizes = {0.1, 0.25, 0.5, 1.0, 2.0};
    CMG_CHECK(cmg::io::write_mtr(path, sizes).has_value());

    auto r = cmg::io::read_mtr(path);
    CMG_CHECK(r.has_value());
    CMG_CHECK(r->size() == sizes.size());
    for (std::size_t i = 0; i < sizes.size(); ++i)
        CMG_CHECK(std::fabs((*r)[i] - sizes[i]) < 1e-9);
}

} // namespace
