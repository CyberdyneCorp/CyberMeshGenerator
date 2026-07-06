# Contributing to CyberMeshGenerator

Thanks for your interest in CyberMeshGenerator — a clean-room Modern C++20 port of TetGen
(quality tetrahedral meshing, 3D Delaunay/Voronoi, voxelization). This guide covers how to
build, the workflow we follow, and what a mergeable change looks like.

## Getting set up

```bash
git clone https://github.com/CyberdyneCorp/CyberMeshGenerator.git
cd CyberMeshGenerator
just build          # configure + build the portable CPU-only baseline
just test           # run the foundation test suite
```

No `just`? The equivalent is plain CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The portable CPU-only build needs only a C++20 compiler and CMake ≥ 3.25 — no external
dependencies. Optional backends/integrations (CUDA, OpenCL, Metal, NumPP, SciPP, Python, Swift)
are all off by default. GPU work: `just gpu-detect`, then `just cuda`.

## Development workflow

We develop spec-first with [OpenSpec](https://github.com/Fission-AI/OpenSpec). Living capability
specs are in [`openspec/specs/`](openspec/specs); completed changes are archived under
[`openspec/changes/archive/`](openspec/changes/archive).

- **Medium or large features** (new algorithms, backends, file formats, bindings, build/packaging
  behavior): start with an OpenSpec change (proposal → specs/tasks) before implementing. Run
  `openspec validate --all --strict` — CI enforces it.
- **Small fixes and docs:** a direct PR is fine. Still keep the specs and docs truthful — if your
  change makes a spec or doc statement false, update it in the same PR.

## What a mergeable PR looks like

- **Tests.** New behavior ships with tests. **Every bug fix includes a regression test** that
  fails before the fix and passes after.
- **Green CI.** The CPU baseline (GCC + Clang, warnings-as-errors), the ASan/UBSan build, the
  single-precision mobile build, the Python binding test, and OpenSpec validation must all pass.
- **Docs/specs in sync.** Update the README and `openspec/specs/` when your change affects
  documented behavior. Don't leave a claim that the code contradicts.
- **Readable, low-complexity code.** Match the surrounding style (`std::span`, strong enums,
  `cmg::expected`, owning containers — no raw owning pointers, no legacy macros). Keep
  per-function cognitive complexity modest; isolate genuinely irreducible algorithms (geometric
  kernels, predicates) and flag them rather than mangling them to hit a number.
- **A descriptive PR message.** Explain what changed and why. If you found a bug, describe how it
  reproduced.

## Numerical correctness

Meshing changes are validated against **TetGen** as a numerical oracle (see the `oracle` recipe
and `openspec/specs/tetgen-oracle`), and GPU paths against the CPU solver with identical output
topology. If you touch predicates, Delaunay insertion, constrained recovery, or refinement, make
sure the relevant oracle/parity tests still pass — and add one if a gap exists.

> **Do not** transcribe TetGen's AGPLv3 source into `src/`. TetGen is used only as a build-time
> test oracle; the algorithms are re-implemented clean-room from the published paper and the
> OpenSpec baseline. See [`NOTICE.md`](NOTICE.md).

## Reporting bugs & requesting features

Open an issue using the templates. For **security** issues, do **not** open a public issue — see
[SECURITY.md](SECURITY.md).

## License

By contributing, you agree that your contributions are licensed under the project's
[MIT License](LICENSE).
