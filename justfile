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

# Build with the CUDA backend and run the suite on the local GPU.
cuda:
    cmake -S . -B build-cuda -DCMAKE_BUILD_TYPE=Release -DCMG_WITH_CUDA=ON
    cmake --build build-cuda -j
    ./build-cuda/tests/cmg_tests

# Build the shared C ABI and run the Python binding test (needs python3 + numpy).
python-test:
    cmake -S . -B build-py -DCMG_BUILD_C_ABI=ON -DCMG_BUILD_SHARED=ON -DCMG_BUILD_TESTS=OFF -DCMG_BUILD_CLI=OFF
    cmake --build build-py -j
    CMG_C_LIB=$(find build-py -name 'libcmg_c.so' | head -1) python3 bindings/python/tests/test_cybermesh.py
