// CyberMeshGenerator — mesh coarsening (vertex removal / decimation).
//
// Removes a deterministic fraction of a mesh's interior vertices and
// re-tetrahedralizes the survivors. Boundary vertices (those on any boundary
// face) are always retained so the domain outline is preserved. Mirrors the
// intent of TetGen's `-R` coarsening pass.
#include "cmg/cmg.hpp"
#include "cmg/coarsen/coarsen.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cmg::coarsen {

namespace {

/// splitmix64: a fast, well-distributed 64-bit PRNG. Advancing the shared state
/// once per interior vertex (in ascending index order) yields a deterministic
/// pseudo-random key for each, fully reproducible from the seed alone.
std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

} // namespace

expected<Mesh, MeshError> coarsen(const Mesh& in, const CoarsenOptions& opts) {
    const std::size_t n = in.points.size();

    // A vertex is a boundary vertex iff it appears in any boundary-face triangle.
    std::vector<char> is_boundary(n, 0);
    for (const auto& f : in.faces)
        for (int v : f)
            if (v >= 0 && static_cast<std::size_t>(v) < n) is_boundary[v] = 1;

    // A vertex is referenced iff it is used by some tetrahedron.
    std::vector<char> is_referenced(n, 0);
    for (const auto& t : in.tetrahedra)
        for (int v : t)
            if (v >= 0 && static_cast<std::size_t>(v) < n) is_referenced[v] = 1;

    // Interior vertices: referenced by a tet but not on the boundary.
    struct Keyed {
        int index;
        std::uint64_t key;
    };
    std::vector<Keyed> interior;
    std::uint64_t state = opts.seed;
    for (std::size_t i = 0; i < n; ++i) {
        if (is_referenced[i] && !is_boundary[i])
            interior.push_back({static_cast<int>(i), splitmix64(state)});
    }

    // Keep the first round(keep_fraction * count) interior vertices by key.
    std::sort(interior.begin(), interior.end(),
              [](const Keyed& a, const Keyed& b) {
                  return a.key != b.key ? a.key < b.key : a.index < b.index;
              });
    double frac = std::clamp(opts.keep_fraction, 0.0, 1.0);
    auto keep_count = static_cast<std::size_t>(
        std::lround(frac * static_cast<double>(interior.size())));
    keep_count = std::min(keep_count, interior.size());

    // Collect kept vertex indices: every boundary vertex + the kept interior ones.
    std::vector<int> kept_indices;
    kept_indices.reserve(n);
    for (std::size_t i = 0; i < n; ++i)
        if (is_boundary[i]) kept_indices.push_back(static_cast<int>(i));
    for (std::size_t i = 0; i < keep_count; ++i)
        kept_indices.push_back(interior[i].index);

    // Stable, deterministic ordering of the retained point set.
    std::sort(kept_indices.begin(), kept_indices.end());

    std::vector<Point3> kept_points;
    kept_points.reserve(kept_indices.size());
    for (int idx : kept_indices) kept_points.push_back(in.points[idx]);

    if (kept_points.size() < 4)
        return unexpected(MeshError{
            MeshErrorCode::InvalidInput,
            "coarsen: fewer than 4 vertices remain; cannot tetrahedralize"});

    return delaunay(kept_points, {});
}

} // namespace cmg::coarsen
