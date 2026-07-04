# NOTICE — provenance and licensing (OPEN — read before distributing)

CyberMeshGenerator is a Modern C++20 port of **TetGen 1.6.0**. Two provenance
questions **must be settled by the maintainer before any public release**. They
are legal decisions, not engineering defaults, and are tracked as blocking items
in the OpenSpec foundation change.

## 1. TetGen license (AGPLv3)

TetGen 1.6.0 is distributed under **AGPLv3** with a commercial dual-license from
the Weierstrass Institute (WIAS). Consequences:

- A **clean-room reimplementation** of TetGen's *algorithms* from the published
  paper (Hang Si, ACM TOMS 41(2), 2015) and the behavioral OpenSpec baseline —
  without copying `tetgen.cxx` — may avoid AGPL obligations. This requires strict
  provenance discipline (no transcription of TetGen source into `src/`).
- **Transcribing** TetGen source makes this a derivative work and inherits AGPLv3
  (or requires a WIAS commercial license).

Decide and document: clean-room vs. licensed derivative, and the resulting license
for this repository. Until then, treat the repository as **not for distribution**.

## 2. Shewchuk predicates

`src/predicates/predicates.cpp` is a **verbatim port** of Jonathan Shewchuk's
`predicates.cxx`, which Shewchuk **placed in the public domain** (see the header
comment in that file). This is distinct from TetGen's AGPL and is generally safe to
reuse, but confirm the exact terms of the bundled version before shipping.

## Attribution

- TetGen — Hang Si, WIAS Berlin. https://codeberg.org/TetGen/TetGen
- Robust geometric predicates — Jonathan R. Shewchuk, CMU (public domain).
