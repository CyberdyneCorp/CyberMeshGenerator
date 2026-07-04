# backend-acceleration Specification

## Purpose
TBD - created by archiving change bootstrap-cybermesh-foundation. Update Purpose after archive.
## Requirements
### Requirement: Reuse NumPP's device dispatch substrate

CyberMeshGenerator SHALL NOT implement a second device-management stack. When a GPU
backend is enabled it SHALL reuse NumPP's `CapabilityRegistry` (compiled-backend and
present-device probing), `last_backend()` introspection, the `NUMPP_GPU_TARGET`
selection override, and the bounded device buffer reuse pool. CyberMeshGenerator GPU
kernels SHALL be registered into the same weak-linked vtable shape NumPP uses.

#### Scenario: Mesher queries the shared registry
- WHEN the mesher needs to decide whether a GPU path is available
- THEN it queries NumPP's `CapabilityRegistry` rather than probing devices itself

#### Scenario: No parallel device runtime
- WHEN CyberMeshGenerator is built with a GPU flag enabled
- THEN device memory and kernel launches go through NumPP's device layer and pool,
  not a CyberMeshGenerator-owned allocator

### Requirement: Accelerated mesh kernels for parallelizable phases

CyberMeshGenerator SHALL provide optional GPU implementations for the parallelizable
phases of meshing — at minimum batched predicate/orientation filtering, BRIO-Hilbert
spatial-sort key computation, spatial point location, and quality-histogram /
worst-tetrahedron scans — each gated by its `CMG_WITH_<BACKEND>` flag, built as a
separate weak-linked translation unit, for **CUDA, OpenCL and Metal**. A portable
CPU implementation SHALL always exist for every such kernel. (oracle: tetgen.cxx
BRIO-Hilbert sort 3272–3338)

#### Scenario: Accelerated kernel present and used above threshold
- GIVEN a build with a GPU backend compiled in and a usable device
- WHEN an eligible accelerated kernel runs on a problem above its size threshold
- THEN the device path is used and `last_backend()` reports that GPU backend

#### Scenario: CPU fallback always available
- GIVEN any accelerated kernel
- WHEN no GPU backend is compiled in, or no device is present, or the problem is
  below the size threshold
- THEN the portable CPU implementation is used and the call still succeeds

### Requirement: Size-threshold-gated dispatch

Each accelerable kernel SHALL choose its implementation from `(operation, problem
size, available backends)`. Below a per-operation size threshold the CPU kernel
SHALL be used to avoid offload overhead; an accelerated backend SHALL be used only
when available and the problem is large enough.

#### Scenario: Small problem stays on CPU
- GIVEN a build with a GPU backend available
- WHEN an accelerable kernel runs below its configured size threshold
- THEN the CPU kernel is used and `last_backend()` reports CPU

#### Scenario: Large problem offloads
- GIVEN a build with a GPU backend available and a usable device
- WHEN an accelerable kernel runs above its size threshold
- THEN the device kernel is used

### Requirement: Backend selection override and Apple preference

CyberMeshGenerator SHALL honor NumPP's `NUMPP_GPU_TARGET` override
(`cpu|cuda|opencl|metal|auto`). `auto` SHALL prefer Metal on Apple platforms and
otherwise try CUDA, then OpenCL, then CPU. Requesting a backend not compiled in
SHALL produce a clear error; `auto` SHALL degrade silently to CPU.

#### Scenario: Force CPU
- GIVEN a build with a GPU backend available
- WHEN `NUMPP_GPU_TARGET=cpu` is set
- THEN all mesh kernels use the CPU path

#### Scenario: Auto prefers Metal on Apple
- GIVEN an Apple build (iOS/macOS) with the Metal backend compiled in and a usable device
- WHEN `NUMPP_GPU_TARGET=auto` is set
- THEN eligible kernels select the Metal backend

#### Scenario: Explicit unavailable backend errors
- GIVEN a build with `CMG_WITH_CUDA=OFF`
- WHEN `NUMPP_GPU_TARGET=cuda` is set and an accelerable kernel is requested on CUDA
- THEN a clear error is reported

### Requirement: Deterministic topology across backends

Enabling a GPU backend SHALL NOT change the combinatorial output of the mesher. The
device path SHALL accelerate candidate generation and filtering only; the
topological mutation and every exact-sign decision SHALL remain on a deterministic
path so that, for the same input and options, the CPU build and any GPU build
produce the same tetrahedralization under the documented tie-breaking, not merely a
numerically-close one.

#### Scenario: CPU and GPU builds agree on topology
- GIVEN the same PLC and options in general position
- WHEN the mesh is generated once on a CPU-only build and once with a GPU backend forced
- THEN the two meshes have the same set of tetrahedra (as sorted vertex tuples)

#### Scenario: Backend choice is observable in tests
- GIVEN an accelerable kernel run above its threshold with a GPU backend forced
- WHEN the selected backend is queried via `last_backend()`
- THEN it reports the GPU backend, letting tests assert the path taken as well as
  the identical topology

### Requirement: Multithreaded CPU acceleration without a GPU

CyberMeshGenerator's CPU kernels for the parallelizable phases SHALL support
optional multithreading, so a desktop CPU without any GPU still accelerates the same
phases the GPU path targets, while the mobile/single-thread build stays correct.
Multithreaded and single-threaded runs SHALL produce the same topology under the
documented tie-breaking.

#### Scenario: Threaded CPU sort matches serial
- GIVEN a large point set
- WHEN the BRIO-Hilbert key computation runs multithreaded and again single-threaded
- THEN both produce the same insertion order and the same resulting topology

#### Scenario: Single-thread mobile build unaffected
- GIVEN a mobile build with threading disabled
- WHEN the mesher runs
- THEN it produces the correct mesh on a single thread

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

