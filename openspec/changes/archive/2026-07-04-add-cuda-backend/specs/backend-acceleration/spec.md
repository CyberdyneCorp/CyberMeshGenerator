# backend-acceleration Specification (CUDA kernel delta)

## ADDED Requirements

### Requirement: CUDA orient3d batch kernel

Under `CMG_WITH_CUDA`, CyberMeshGenerator SHALL provide a CUDA device kernel
`cmg::backend::cuda::orient3d_signs` that computes, on the GPU, the sign of the
double-precision orient3d determinant for each candidate tetrahedron in a batch, and a
runtime probe `cmg::backend::cuda::available()`. The CUDA build SHALL NOT require
NumPP and SHALL be forward-compatible (PTX) so it runs on GPUs newer than the build
toolchain natively targets. (oracle: backend-acceleration; NVIDIA CUDA)

#### Scenario: GPU signs match the CPU predicate
- GIVEN a batch of candidate tetrahedra in general position and a present CUDA device
- WHEN `orient3d_signs` runs on the GPU
- THEN each returned sign equals the sign the CPU `orient3d` gives for that candidate

### Requirement: CUDA dispatch with exact CPU fallback

`robust::orient3d_batch` SHALL use the CUDA kernel when compiled with CUDA, a device
is present, and the batch exceeds the offload threshold; candidates whose GPU sign is
uncertain (near-zero determinant) SHALL be escalated to the exact CPU predicate, and
the final result SHALL be identical to the pure-CPU batch. When CUDA is not compiled or
no device is present, the CPU path SHALL be used unchanged.

#### Scenario: Batched result identical with and without the GPU
- GIVEN the same candidate batch
- WHEN it is evaluated once on a CPU-only build and once with the CUDA backend
- THEN the two sign arrays are identical (exact escalation covers the uncertain cases)

#### Scenario: Backend reported
- GIVEN a CUDA build with a device and an above-threshold batch
- WHEN the batch runs
- THEN `last_backend()` reports CUDA
