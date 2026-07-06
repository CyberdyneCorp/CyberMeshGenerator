from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout


class CyberMeshGeneratorConan(ConanFile):
    name = "cybermeshgenerator"
    version = "0.5.0"
    description = "Modern C++20 port of TetGen with CPU/CUDA/OpenCL/Metal acceleration"
    homepage = "https://github.com/CyberdyneCorp/CyberMeshGenerator"
    license = "SEE-LICENSE-IN-LICENSE"  # AGPLv3 vs. relicensing is an open decision
    settings = "os", "compiler", "build_type", "arch"

    # Every backend/integration is OFF by default: the portable CPU-only build is
    # the mobile (iOS/Android) baseline and needs no extra dependency.
    options = {
        "shared": [True, False],
        "with_cuda": [True, False],
        "with_opencl": [True, False],
        "with_metal": [True, False],
        "with_numpp": [True, False],
        "with_scipp": [True, False],
        "single": [True, False],
    }
    default_options = {
        "shared": False,
        "with_cuda": False,
        "with_opencl": False,
        "with_metal": False,
        "with_numpp": False,
        "with_scipp": False,
        "single": False,
    }

    exports_sources = "CMakeLists.txt", "include/*", "src/*", "tests/*", "cmake/*"

    def requirements(self):
        # NumPP is pulled only when integrating, and is required transitively by
        # any GPU backend (device dispatch is routed through it).
        gpu = self.options.with_cuda or self.options.with_opencl or self.options.with_metal
        if self.options.with_numpp or gpu:
            self.requires("numpp/1.6.0")
        if self.options.with_scipp:
            self.requires("scipp/0.1.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        gpu = self.options.with_cuda or self.options.with_opencl or self.options.with_metal
        tc.variables["CMG_WITH_CUDA"] = bool(self.options.with_cuda)
        tc.variables["CMG_WITH_OPENCL"] = bool(self.options.with_opencl)
        tc.variables["CMG_WITH_METAL"] = bool(self.options.with_metal)
        tc.variables["CMG_WITH_NUMPP"] = bool(self.options.with_numpp) or bool(gpu)
        tc.variables["CMG_WITH_SCIPP"] = bool(self.options.with_scipp)
        tc.variables["CMG_SINGLE"] = bool(self.options.single)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["cmg"]
