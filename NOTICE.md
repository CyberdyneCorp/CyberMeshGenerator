# NOTICE — provenance and licensing

CyberMeshGenerator is a Modern C++20 port of **TetGen 1.6.0**. It is released under
the **MIT License** (see [`LICENSE`](LICENSE)) as a **clean-room reimplementation**:
its algorithms are implemented from the published paper (Hang Si, ACM TOMS 41(2),
2015) and a behavioral OpenSpec baseline, **without transcribing TetGen source**
(`tetgen.cxx`) into `src/`. TetGen is used only as a behavioral *oracle* in the tests.

## 1. TetGen (AGPLv3) — clean-room, not a derivative

TetGen 1.6.0 is distributed under **AGPLv3** with a commercial dual-license from the
Weierstrass Institute (WIAS). A clean-room reimplementation of its *algorithms* — with
no copying of TetGen source — is an independent work and does not inherit AGPLv3.
CyberMeshGenerator is maintained under that discipline: no transcription of TetGen
source into `src/`. If you contribute, keep to it — implement from specs/papers, not by
copying `tetgen.cxx`.

## 2. Shewchuk predicates

`src/predicates/predicates.cpp` is a **verbatim port** of Jonathan Shewchuk's
`predicates.cxx`, which Shewchuk **placed in the public domain** (see the header comment
in that file). Public-domain code is freely incorporable, including into an MIT-licensed
work; it retains its public-domain origin and attribution below.

## Attribution

- TetGen — Hang Si, WIAS Berlin. https://codeberg.org/TetGen/TetGen (algorithms; oracle)
- Robust geometric predicates — Jonathan R. Shewchuk, CMU (public domain).
