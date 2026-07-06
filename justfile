# CyberMeshGenerator developer tasks — run `just` (or `just --list`) to see all.
# Requires: cmake >= 3.25, clang++ or g++ (C++20). Optional: TetGen at
# /home/leonardo/work/TetGen for the live oracle, python3 for the Python binding.

build_dir := "build"

# Show the available recipes (default).
default:
    @just --list

# Detect usable GPU backends (CUDA / OpenCL / Metal) and recommend a recipe.
gpu-detect:
    #!/usr/bin/env bash
    set -uo pipefail
    os=$(uname -s); arch=$(uname -m)
    case "$os" in
      MINGW*|MSYS*|CYGWIN*) plat=Windows ;;
      Darwin)               plat=macOS   ;;
      Linux)                plat=Linux   ;;
      *)                    plat="$os"   ;;
    esac
    echo "Host: $plat ($os $arch)"
    echo
    cuda=no; opencl=no; metal=no

    # --- CUDA (NVIDIA) — driver at runtime, nvcc to build ----------------------
    if command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi -L >/dev/null 2>&1; then
      gpu=$(nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | head -1)
      drv=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null | head -1)
      if command -v nvcc >/dev/null 2>&1; then
        nvcc=$(nvcc --version | grep -oE 'release [0-9.]+' | head -1)
      else
        nvcc="nvcc not on PATH — install the CUDA toolkit to build"
      fi
      echo "✅ CUDA   : ${gpu:-NVIDIA GPU} (driver ${drv:-?}, ${nvcc})"
      cuda=yes
    else
      echo "❌ CUDA   : no NVIDIA driver (nvidia-smi) detected"
    fi

    # --- OpenCL — clinfo if present, else probe the ICD loader / framework -----
    if command -v clinfo >/dev/null 2>&1 && [ "$(clinfo -l 2>/dev/null | grep -ciE 'device|platform')" -gt 0 ]; then
      dev=$(clinfo -l 2>/dev/null | grep -iE 'device' | head -1 | sed 's/^[[:space:]]*//')
      echo "✅ OpenCL : ${dev:-device present} (clinfo)"; opencl=yes
    elif [ "$plat" = macOS ] && [ -d /System/Library/Frameworks/OpenCL.framework ]; then
      echo "✅ OpenCL : Apple OpenCL.framework present (install clinfo for details)"; opencl=yes
    elif [ "$plat" = Windows ] && { [ -f /c/Windows/System32/OpenCL.dll ] || reg query "HKLM\\SOFTWARE\\Khronos\\OpenCL\\Vendors" >/dev/null 2>&1; }; then
      echo "✅ OpenCL : Windows OpenCL ICD present (install clinfo to enumerate)"; opencl=yes
    elif ls /etc/OpenCL/vendors/*.icd >/dev/null 2>&1 || ldconfig -p 2>/dev/null | grep -q libOpenCL; then
      echo "✅ OpenCL : ICD loader present (install clinfo to enumerate devices)"; opencl=yes
    else
      echo "❌ OpenCL : no ICD loader / OpenCL runtime detected"
    fi

    # --- Metal (Apple GPU) — Apple platforms only ------------------------------
    if [ "$plat" = macOS ] && [ -d /System/Library/Frameworks/Metal.framework ]; then
      dev=$(system_profiler SPDisplaysDataType 2>/dev/null | grep -iE 'Chipset Model' | head -1 | sed 's/^[[:space:]]*//')
      echo "✅ Metal  : ${dev:-Metal framework present}"; metal=yes
    else
      echo "❌ Metal  : Apple platforms only"
    fi

    echo
    # CUDA ships real device kernels today; OpenCL/Metal are scaffolded behind the
    # dispatch layer, so the CPU baseline is the safe recommendation for those.
    if   [ "$cuda"  = yes ]; then rec="CUDA →  just cuda    (real device kernels)"
    elif [ "$metal" = yes ] || [ "$opencl" = yes ]; then
      rec="CPU only →  just test   (OpenCL/Metal are scaffolded; CPU is always available)"
    else rec="CPU only →  just test   (portable CPU backend; always available)"; fi
    echo "Recommended: $rec"

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
