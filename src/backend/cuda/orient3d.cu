// CyberMeshGenerator — CUDA orient3d batch fast-filter kernel.
//
// One thread per candidate tetrahedron computes the double-precision Shewchuk
// orient3d determinant and returns its sign. A near-zero determinant yields 0,
// which the caller escalates to the exact CPU predicate.
#include "cmg/backend/cuda_kernels.hpp"

#include <cuda_runtime.h>

namespace cmg::backend::cuda {

namespace {

__global__ void orient3d_signs_kernel(const double* __restrict__ xyz,
                                       const int* __restrict__ tets, int n,
                                       int* __restrict__ out_sign) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    const int ia = tets[4 * i + 0];
    const int ib = tets[4 * i + 1];
    const int ic = tets[4 * i + 2];
    const int id = tets[4 * i + 3];

    const double ax = xyz[3 * ia + 0], ay = xyz[3 * ia + 1], az = xyz[3 * ia + 2];
    const double bx = xyz[3 * ib + 0], by = xyz[3 * ib + 1], bz = xyz[3 * ib + 2];
    const double cx = xyz[3 * ic + 0], cy = xyz[3 * ic + 1], cz = xyz[3 * ic + 2];
    const double dx = xyz[3 * id + 0], dy = xyz[3 * id + 1], dz = xyz[3 * id + 2];

    const double adx = ax - dx, ady = ay - dy, adz = az - dz;
    const double bdx = bx - dx, bdy = by - dy, bdz = bz - dz;
    const double cdx = cx - dx, cdy = cy - dy, cdz = cz - dz;

    const double t0 = adx * (bdy * cdz - bdz * cdy);
    const double t1 = ady * (bdx * cdz - bdz * cdx);
    const double t2 = adz * (bdx * cdy - bdy * cdx);
    const double det = t0 - t1 + t2;

    // Relative epsilon: scale a tiny tolerance by the magnitude of the terms so
    // that only determinants that are effectively zero collapse to sign 0.
    const double permanent = fabs(t0) + fabs(t1) + fabs(t2);
    const double tol = 1e-10 * permanent;

    out_sign[i] = (det > tol) ? 1 : (det < -tol) ? -1 : 0;
}

} // namespace

bool available() {
    int n = 0;
    return cudaGetDeviceCount(&n) == cudaSuccess && n > 0;
}

void orient3d_signs(const double* xyz, int npts, const int* tets, int n,
                    int* out_sign) {
    if (n <= 0) return;

    double* d_xyz = nullptr;
    int* d_tets = nullptr;
    int* d_out = nullptr;

    const size_t xyz_bytes = static_cast<size_t>(npts) * 3 * sizeof(double);
    const size_t tets_bytes = static_cast<size_t>(n) * 4 * sizeof(int);
    const size_t out_bytes = static_cast<size_t>(n) * sizeof(int);

    cudaMalloc(&d_xyz, xyz_bytes);
    cudaMalloc(&d_tets, tets_bytes);
    cudaMalloc(&d_out, out_bytes);

    cudaMemcpy(d_xyz, xyz, xyz_bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_tets, tets, tets_bytes, cudaMemcpyHostToDevice);

    const int block = 256;
    const int grid = (n + block - 1) / block;
    orient3d_signs_kernel<<<grid, block>>>(d_xyz, d_tets, n, d_out);

    cudaMemcpy(out_sign, d_out, out_bytes, cudaMemcpyDeviceToHost);

    cudaFree(d_xyz);
    cudaFree(d_tets);
    cudaFree(d_out);
}

} // namespace cmg::backend::cuda
