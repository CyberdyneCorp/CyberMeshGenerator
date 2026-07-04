// CyberMeshGenerator — backend dispatch shim implementation.
//
// On a CPU-only build (no NumPP / no GPU backend) every op resolves to the CPU
// kernel and last_backend() is always Cpu. When CMG_WITH_NUMPP is on and a GPU
// backend is compiled in, should_offload() consults NumPP's CapabilityRegistry
// and the per-op size threshold; the device kernels themselves are registered by
// the Phase 11 backend change. This foundation TU establishes the contract and
// the CPU-only behavior.
#include "cmg/backend/dispatch.hpp"

#if CMG_WITH_NUMPP
#include "numpp/backend/capability_registry.hpp"
#endif
#if CMG_WITH_CUDA
#include "cmg/backend/cuda_kernels.hpp"
#endif

namespace cmg::backend {

namespace {

// Per-op minimum problem size below which offloading never pays for itself.
std::size_t threshold_for(Op op) noexcept {
    switch (op) {
        case Op::OrientFilter:  return 1u << 16; // 65k candidates
        case Op::HilbertSort:   return 1u << 15;
        case Op::PointLocation: return 1u << 16;
        case Op::QualityScan:   return 1u << 17;
    }
    return static_cast<std::size_t>(-1);
}

// Records the backend that served the most recent op, per thread, so tests can
// assert the path taken.
thread_local Backend g_last = Backend::Cpu;

} // namespace

bool should_offload(Op op, std::size_t problem_size) noexcept {
    if (problem_size < threshold_for(op)) {
        g_last = Backend::Cpu;
        return false;
    }
#if CMG_WITH_CUDA
    // cmg's own CUDA device kernel: offload above the threshold when a device is
    // present. (Checked before the NumPP registry so the built-in kernel is used.)
    if (backend::cuda::available()) {
        g_last = Backend::Cuda;
        return true;
    }
#endif
#if CMG_WITH_NUMPP && (CMG_WITH_OPENCL || CMG_WITH_METAL)
    // Defer to NumPP's registry for other backends (compiled-backend + present-
    // device probing, NUMPP_GPU_TARGET override, Metal-first on Apple).
    if (numpp::backend::capability_registry().has_usable_device()) {
        g_last = static_cast<Backend>(
            numpp::backend::capability_registry().selected_backend());
        return g_last != Backend::Cpu;
    }
#endif
    g_last = Backend::Cpu;
    return false;
}

Backend last_backend() noexcept { return g_last; }

std::string_view name(Backend b) noexcept {
    switch (b) {
        case Backend::Cpu:    return "cpu";
        case Backend::Cuda:   return "cuda";
        case Backend::OpenCL: return "opencl";
        case Backend::Metal:  return "metal";
    }
    return "cpu";
}

} // namespace cmg::backend
