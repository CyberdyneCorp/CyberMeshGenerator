# Tasks — CUDA backend (backend-acceleration)
- [x] `src/backend/cuda/orient3d.cu`: available() + orient3d_signs kernel (double orient3d determinant sign per candidate); build via CMG_WITH_CUDA, PTX-forward (80-virtual)
- [x] Dispatch: robust::orient3d_batch uses CUDA when available+above threshold; exact CPU escalation for near-zero signs; identical result to CPU
- [x] last_backend() reports CUDA on the GPU path
- [x] Tests: GPU signs match CPU exact signs on random general-position batches; batched result identical CPU-vs-GPU
- [x] CPU-only build unchanged; CUDA build green + runs on the device

## Deferred
- [ ] OpenCL / Metal kernels (no device / Apple-only here); GPU insertion; NumPP registry integration
