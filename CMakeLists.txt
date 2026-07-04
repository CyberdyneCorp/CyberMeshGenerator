cmake_minimum_required(VERSION 3.25)

project(CyberMeshGenerator
    VERSION 0.1.0
    DESCRIPTION "Modern C++20 port of TetGen with CPU/CUDA/OpenCL/Metal acceleration"
    HOMEPAGE_URL "https://github.com/CyberdyneCorp/CyberMeshGenerator"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Options — every CMG_WITH_* defaults OFF. A build with all of them OFF is the
# portable mobile (iOS/Android) baseline and MUST be fully functional using only
# the C++ standard library.
# -----------------------------------------------------------------------------
option(CMG_BUILD_TESTS   "Build the test suite"                   ON)
option(CMG_BUILD_SHARED  "Build shared library"                   OFF)
option(CMG_BUILD_C_ABI   "Build the stable C ABI shim"            OFF)
option(CMG_ENABLE_ASAN   "Enable AddressSanitizer/UBSan"          OFF)
option(CMG_WARNINGS_AS_ERRORS "Treat warnings as errors"          OFF)

# Optional GPU acceleration backends (routed through NumPP's device layer).
option(CMG_WITH_CUDA     "Enable optional CUDA GPU backend"       OFF)
option(CMG_WITH_OPENCL   "Enable optional OpenCL GPU backend"     OFF)
option(CMG_WITH_METAL    "Enable optional Metal GPU backend"      OFF)

# Optional integrations with the CyberdyneCorp scientific stack.
option(CMG_WITH_NUMPP    "Integrate NumPP (interop + device dispatch)" OFF)
option(CMG_WITH_SCIPP    "Integrate SciPP (spatial primitives)"   OFF)

# Optional language bindings.
option(CMG_WITH_PYTHON   "Build the Python bindings"              OFF)
option(CMG_WITH_SWIFT    "Build the Swift C-ABI shim for SwiftPM" OFF)

# Optional multithreaded CPU acceleration for the parallelizable phases.
option(CMG_WITH_THREADS  "Enable the multithreaded CPU path"      OFF)

# Numerical precision: ON selects float REAL (mirrors TetGen -DSINGLE).
option(CMG_SINGLE        "Use single-precision (float) REAL"      OFF)

# -----------------------------------------------------------------------------
# Flag reconciliation: a GPU backend requires the NumPP device substrate.
# Enabling a GPU flag implies CMG_WITH_NUMPP and the matching NumPP backend flag;
# configuration fails fast if NumPP is missing or lacks that backend.
# -----------------------------------------------------------------------------
if(CMG_WITH_CUDA OR CMG_WITH_OPENCL OR CMG_WITH_METAL)
    if(NOT CMG_WITH_NUMPP)
        message(STATUS "A GPU backend was requested; enabling CMG_WITH_NUMPP "
                       "(GPU dispatch is routed through NumPP).")
        set(CMG_WITH_NUMPP ON CACHE BOOL "" FORCE)
    endif()
endif()

# -----------------------------------------------------------------------------
# Project-wide settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMG_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
endif()

# Toolchain guard: fail early with a clear message on a pre-C++20 compiler.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
    message(FATAL_ERROR "CyberMeshGenerator requires clang >= 14 (found ${CMAKE_CXX_COMPILER_VERSION})")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
    message(FATAL_ERROR "CyberMeshGenerator requires GCC >= 11 (found ${CMAKE_CXX_COMPILER_VERSION})")
endif()

# -----------------------------------------------------------------------------
# Optional dependencies (pulled only when the integration flag is ON).
# -----------------------------------------------------------------------------
if(CMG_WITH_NUMPP)
    find_package(NumPP CONFIG REQUIRED)
endif()
if(CMG_WITH_SCIPP)
    find_package(SciPP CONFIG REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Generated headers
# -----------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/cmg/backend/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/cmg/backend/config.hpp" @ONLY)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/cmg/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/cmg/version.hpp" @ONLY)

if(CMG_BUILD_TESTS)
    enable_testing()
endif()

# -----------------------------------------------------------------------------
# Library target
# -----------------------------------------------------------------------------
add_library(cmg
    src/core/options.cpp
    src/core/tetrahedralize.cpp
    src/delaunay/incremental.cpp
    src/constrained/tetrahedralize_plc.cpp
    src/backend/dispatch.cpp
    src/predicates/robust.cpp
    src/predicates/predicates.cpp
    src/io/dispatch.cpp
    src/io/node.cpp
    src/io/ele.cpp
    src/io/faceedge.cpp
    src/io/poly.cpp
    src/io/stl.cpp
    src/io/off.cpp
    src/io/ply.cpp
    src/io/vtk.cpp
    src/io/medit.cpp
)
add_library(cmg::cmg ALIAS cmg)

target_compile_features(cmg PUBLIC cxx_std_20)
target_include_directories(cmg PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
    "$<INSTALL_INTERFACE:include>"
)

if(CMG_WITH_NUMPP)
    target_link_libraries(cmg PUBLIC NumPP::numpp)
endif()
if(CMG_WITH_SCIPP)
    target_link_libraries(cmg PUBLIC SciPP::scipp)
endif()
if(CMG_WITH_THREADS)
    find_package(Threads REQUIRED)
    target_link_libraries(cmg PUBLIC Threads::Threads)
endif()

# -----------------------------------------------------------------------------
# CORRECTNESS INVARIANT: the Shewchuk predicates must be compiled at -O0.
# Aggressive FP reassociation/contraction breaks their adaptive error analysis
# and would produce wrong signs on degenerate input. This is NOT a perf choice.
# (oracle: TetGen makefile `PREDCXXFLAGS = -O0`)
# -----------------------------------------------------------------------------
# The verbatim Shewchuk port is exempt from -Wall/-Werror (it is transcribed
# third-party code, not ours to restyle) via -w, and pinned to -O0.
set_source_files_properties(src/predicates/predicates.cpp PROPERTIES
    COMPILE_OPTIONS "-O0;-w")

if(CMG_ENABLE_ASAN)
    target_compile_options(cmg PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(cmg PUBLIC -fsanitize=address,undefined)
endif()

if(CMG_WARNINGS_AS_ERRORS)
    target_compile_options(cmg PRIVATE -Wall -Wextra -Werror)
endif()

# -----------------------------------------------------------------------------
# C ABI shim (foundation for the Python/Swift bindings). Built when either
# binding is requested, or when explicitly forced. Needs C enabled.
# -----------------------------------------------------------------------------
if(CMG_WITH_PYTHON OR CMG_WITH_SWIFT OR CMG_BUILD_C_ABI)
    enable_language(C)
    add_subdirectory(bindings/c)
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
if(CMG_BUILD_TESTS)
    add_subdirectory(tests)
endif()
