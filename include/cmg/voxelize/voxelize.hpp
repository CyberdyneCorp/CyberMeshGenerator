// CyberMeshGenerator — solid voxelization (occupancy grid + signed-distance field).
//
// Rasterizes a closed PLC boundary into a regular axis-aligned grid: each cell is
// classified inside/outside by an EXACT vertical ray-parity test built on Shewchuk's
// orient3d (not an epsilon ray cast), so a watertight mesh classifies without
// boundary speckle. Occupancy mode stores an inside/outside byte per cell;
// SignedDistance mode stores the signed distance from the cell center to the nearest
// boundary triangle (negative inside). Fidelity is bounded by the input tessellation —
// this is a mesh engine, not a B-Rep/CAD kernel (see the change proposal's OCCT
// positioning): a curved surface is only as accurate as its faceting.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/plc.hpp"

namespace cmg::voxelize {

/// What each cell stores: an inside/outside byte, or a signed distance field.
enum class VoxelMode { Occupancy, SignedDistance };

struct VoxelOptions {
    /// Number of cubic cells along the LONGEST bounding-box axis (fewer along the
    /// shorter axes). Higher = finer. Must be >= 1.
    int resolution = 64;
    /// Margin cells added on every side of the (unpadded) bounding box.
    int pad = 1;
    VoxelMode mode = VoxelMode::Occupancy;
};

/// A dense axis-aligned regular grid produced by voxelize(). Cells are cubic with
/// side `spacing`. Cell (i,j,k) is centered at
/// origin + (i,j,k)*spacing, and its flat index is idx = (k*ny + j)*nx + i.
struct VoxelGrid {
    Point3 origin;      ///< world coord of the CENTER of cell (0,0,0)
    double spacing = 0; ///< cubic cell size (world units)
    int nx = 0, ny = 0, nz = 0;

    /// Occupancy bytes (Occupancy mode): 1 = inside the solid, 0 = outside.
    std::vector<std::uint8_t> occupancy;
    /// Signed distances (SignedDistance mode): negative inside, positive outside.
    std::vector<float> distance;

    /// Row-major flat index of cell (i,j,k): idx = (k*ny + j)*nx + i.
    std::size_t index(int i, int j, int k) const {
        return (static_cast<std::size_t>(k) * ny + j) * nx + i;
    }
    std::size_t cell_count() const {
        return static_cast<std::size_t>(nx) * ny * nz;
    }
};

/// Voxelize a closed PLC into a regular grid. The grid is axis-aligned over the PLC
/// bounding box expanded by `opts.pad` cells, with `opts.resolution` cubic cells along
/// the longest axis. Deterministic for a given PLC and options. Rejects
/// `resolution < 1`, an empty PLC, and a degenerate (zero-extent) PLC with
/// MeshErrorCode::InvalidInput.
expected<VoxelGrid, MeshError> voxelize(const PLC& plc, const VoxelOptions& opts = {});

} // namespace cmg::voxelize
