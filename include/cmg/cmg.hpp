// CyberMeshGenerator — umbrella header.
//
// Modern C++20 port of TetGen: a Delaunay-based quality tetrahedral mesh
// generator that runs on pure CPU, GPU desktops, and mobile (iOS/Android), with
// optional CUDA/OpenCL/Metal acceleration and Python/Swift bindings.
//
// Include this to get the full public API.
#pragma once

#include "cmg/api.hpp"
#include "cmg/backend/config.hpp"
#include "cmg/coarsen/coarsen.hpp"
#include "cmg/reconstruct/reconstruct.hpp"
#include "cmg/backend/dispatch.hpp"
#include "cmg/core/error.hpp"
#include "cmg/core/expected.hpp"
#include "cmg/core/geometry.hpp"
#include "cmg/core/mesh.hpp"
#include "cmg/core/options.hpp"
#include "cmg/core/plc.hpp"
#include "cmg/optimize/smooth.hpp"
#include "cmg/predicates/robust.hpp"
#include "cmg/sizing/background.hpp"
#include "cmg/version.hpp"
#include "cmg/voronoi/voronoi.hpp"
