# Add a real CUDA device kernel (backend-acceleration)

## Why

The backend-acceleration architecture (Phase 0) established the dispatch contract but
shipped no actual GPU kernels. A CUDA-capable GPU (NVIDIA RTX 5060, CUDA 12.0) is
available in this environment, so we can deliver — and **test on real hardware** — the
first genuine device kernel: batched orient3d filtering, the parallelizable hot path
of point location and insertion.

## What changes

Spec delta for the **backend-acceleration** capability:

- `cmg::backend::cuda::orient3d_signs(xyz, npts, tets, n, out)` — a CUDA kernel that
  computes, per candidate tetrahedron, the sign of the double-precision orient3d
  determinant on the GPU. Built only with `-DCMG_WITH_CUDA=ON`.
- `cmg::backend::cuda::available()` — runtime device probe.
- `robust::orient3d_batch` dispatches to the CUDA kernel when compiled with CUDA, a
  device is present, and the batch is above the size threshold; near-zero (uncertain)
  signs are **escalated to the exact CPU predicate**, so the batched result is
  identical to the pure-CPU path.
- CUDA is cmg's own kernel (no NumPP required); PTX-forward build (`80-virtual`) so it
  JIT-runs on newer GPUs than the nvcc toolchain natively targets.

## Impact

- With `CMG_WITH_CUDA=ON`, large orient3d batches run on the GPU and
  `last_backend()` reports CUDA; results match the CPU path exactly (exact escalation
  for the uncertain minority). The default CPU-only build is unchanged.

## Non-goals
- **OpenCL / Metal kernels** — the same kernel structure applies; OpenCL has no
  confirmed device here and Metal is Apple-only, so both are scaffolded/deferred,
  not shipped tested.
- **GPU insertion / full mesh generation on device** — only the batched predicate
  filter is offloaded; topological mutation stays on the deterministic CPU path.
- **NumPP registry integration** — cmg ships its own kernel; sharing NumPP's device
  registry remains a separate opt-in.
