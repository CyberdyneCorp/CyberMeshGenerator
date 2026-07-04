# CyberMeshGenerator developer tasks — run `just` (or `just --list`) to see all.
# Requires: cmake >= 3.25, clang++ or g++ (C++20). Optional: TetGen at
# /home/leonardo/work/TetGen for the live oracle, python3 for the Python binding.

build_dir := "build"

# Show the available recipes (default).
default:
    @just --list

# Configure the portable CPU-only build (all backends/integrations OFF).
configure *FLAGS:
    cmake -S . -B {{build_dir}} -DCMAKE_BUILD_TYPE=Release {{FLAGS}}

# Build the library and tests.
build: configure
    cmake --build {{build_dir}} -j

# Build only the library.
lib: configure
    cmake --build {{build_dir}} --target cmg -j

# Run the foundation test suite.
test: build
    ./{{build_dir}}/tests/cmg_tests

# Run the suite through CTest.
ctest: build
    ctest --test-dir {{build_dir}} --output-on-failure

# Build + test under AddressSanitizer/UBSan (separate build dir).
asan:
    cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DCMG_ENABLE_ASAN=ON
    cmake --build build-asan -j
    ./build-asan/tests/cmg_tests

# Run the TetGen oracle comparison (frozen mode; no TetGen build required).
oracle: build
    CMG_ORACLE_FROZEN=1 ./{{build_dir}}/tests/cmg_tests

# Validate the OpenSpec change(s).
spec:
    openspec validate --all --strict

# Everything CI runs: spec validation + a warnings-as-errors build + tests.
ci:
    openspec validate --all --strict
    cmake -S . -B build-ci -DCMAKE_BUILD_TYPE=Release -DCMG_WARNINGS_AS_ERRORS=ON
    cmake --build build-ci -j
    ctest --test-dir build-ci --output-on-failure

# Configure the mobile baseline (single precision, single-threaded, no deps).
mobile:
    cmake -S . -B build-mobile -DCMAKE_BUILD_TYPE=Release -DCMG_SINGLE=ON
    cmake --build build-mobile -j
    ./build-mobile/tests/cmg_tests

# Remove all build directories.
clean:
    rm -rf build build-*
