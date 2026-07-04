// CyberMeshGenerator — backend dispatch shim.
//
// CyberMeshGenerator does NOT own a device runtime. When CMG_WITH_NUMPP is on and
// a GPU backend is compiled in, the accelerable phases (batched predicate
// filtering, BRIO-Hilbert key computation, point location, quality scans) route
// through NumPP's CapabilityRegistry / last_backend() / NUMPP_GPU_TARGET / device
// buffer pool. With NumPP off, every op resolves to the portable CPU kernel.
#pragma once

#include <string_view>

#include "cmg/backend/config.hpp"

namespace cmg::backend {

/// The compute backend that actually served the most recent accelerable op.
enum class Backend { Cpu, Cuda, OpenCL, Metal };

/// Accelerable operations the dispatcher knows about. Below a per-op size
/// threshold, or when no device is available, each falls back to the CPU kernel.
enum class Op {
    OrientFilter,   ///< batched orient3d/insphere fast filter
    HilbertSort,    ///< BRIO-Hilbert spatial-sort key computation
    PointLocation,  ///< spatial point location
    QualityScan,    ///< quality-histogram / worst-tet reduction
};

/// True if `op` at the given problem size should use a GPU backend on this build:
/// requires a compiled+present device (queried from NumPP's registry) and a size
/// at or above the op's threshold. Always false on a CPU-only build.
bool should_offload(Op op, std::size_t problem_size) noexcept;

/// The backend that served the most recent accelerable op on this thread. Lets
/// tests assert the path taken (not just the numeric result). Reports Cpu on a
/// CPU-only build. Mirrors NumPP's last_backend().
Backend last_backend() noexcept;

/// Human-readable backend name (e.g. for diagnostics and NUMPP_GPU_TARGET echo).
std::string_view name(Backend b) noexcept;

} // namespace cmg::backend
