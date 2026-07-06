# Security Policy

CyberMeshGenerator is a geometry/meshing library. The most relevant risks are memory-safety
issues (out-of-bounds access, use-after-free) reachable through the C ABI or the mesh/geometry
import path (STL / OBJ / OFF / PLY / `.poly` / `.node` parsing), and denial-of-service from
untrusted input (e.g. a malformed surface, a degenerate PLC, or an extreme voxel resolution).

## Supported versions

The project is pre-1.0 (`0.x`). Security fixes are applied to `main` and released in the next
tag. There is no long-term-support branch yet.

| Version | Supported |
|---------|-----------|
| `main`  | ✅ |
| latest `0.x` tag | ✅ |
| older tags | ❌ |

## Reporting a vulnerability

**Please do not open a public issue for security vulnerabilities.**

Report privately via GitHub's [**Report a vulnerability**](https://github.com/CyberdyneCorp/CyberMeshGenerator/security/advisories/new)
(Security → Advisories), or email the maintainer at **leonardoaraujo.santos@gmail.com** with:

- a description of the issue and its impact,
- steps or a minimal input that reproduces it (a mesh/surface file, PLC, switch string, or a code
  snippet driving the C++ API or the C ABI),
- affected version/commit.

We aim to acknowledge a report within a few days and to agree on a disclosure timeline with you.
Please give us a reasonable window to ship a fix before any public disclosure.

## Scope

In scope: the core library, the C ABI, the file-format importers/exporters, and the language
bindings in this repo. Out of scope: vulnerabilities in third-party dependencies (report those
upstream) and issues that require running deliberately hostile build tooling.
