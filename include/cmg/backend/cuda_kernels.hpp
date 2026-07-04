// CyberMeshGenerator — CUDA device kernels (built only with CMG_WITH_CUDA).
//
// Real GPU implementations of the parallelizable meshing hot path. Present only
// when compiled with CUDA; callers guard use behind `#if CMG_WITH_CUDA` and the
// runtime `available()` probe, and always fall back to the exact CPU path.
#pragma once

namespace cmg::backend::cuda {

/// True if a usable CUDA device is present at runtime.
bool available();

/// Compute, on the GPU, the sign (-1, 0, or +1) of the double-precision orient3d
/// determinant for each of `n` candidate tetrahedra. `xyz` holds 3 doubles per
/// point for `npts` points; `tets` holds 4 vertex indices per candidate. Writes
/// `n` signs to `out_sign`. The sign matches the CPU double-precision orient3d for
/// non-degenerate inputs; a near-zero determinant yields 0, which the caller
/// escalates to the exact CPU predicate. This is the batched fast-filter the
/// point-location and insertion phases use.
void orient3d_signs(const double* xyz, int npts, const int* tets, int n,
                    int* out_sign);

} // namespace cmg::backend::cuda
