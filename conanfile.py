from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
from conan.errors import ConanInvalidConfiguration
import os

class AniStudio(ConanFile):
    name = "AniStudio"
    version = "0.1.0"
    
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_cuda": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_cuda": False
    }

    def build_requirements(self):
        # Simplified CMake requirement
        self.tool_requires("cmake/[>=3.25]")

    def requirements(self):
        self.requires("opencv/4.5.5")
        self.requires("glfw/3.4")
        self.requires("glew/2.1.0")
        self.requires("zlib/1.2.11")
        self.requires("libpng/1.6.44")
        self.requires("exiv2/0.28.1")
        self.requires("ffmpeg/4.4.4")

        if self.settings.os == "Windows":
            self.requires("opengl/system")
        elif self.settings.os == "Linux":
            self.requires("opengl/system")
            self.requires("xorg/system")
        elif self.settings.os == "Macos":
            self.requires("opengl/system")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        if self.settings.compiler.get_safe("cppstd"):
            self.settings.compiler.cppstd = "17"
        
        # Windows-specific configuration
        if self.settings.os == "Windows":
            self.conf.define("tools.microsoft.msbuild:vs_version", "17")
            self.conf.define("tools.cmake.cmaketoolchain:generator", "Visual Studio 17 2022")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "17"
        tc.variables["CMAKE_CXX_STANDARD_REQUIRED"] = "ON"
        tc.variables["CMAKE_CXX_EXTENSIONS"] = "OFF"
        tc.variables["WITH_CUDA"] = self.options.with_cuda
        tc.generate()
        
        deps = CMakeDeps(self)
        deps.generate()

    # [Rest of your methods remain unchanged]
    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = os.path.join(self.folders.build, "conan")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE*", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        
    def package_info(self):
        self.cpp_info.libs = ["media_creation_tool"]
        if self.settings.os == "Linux":
            self.cpp_info.system_libs.extend(["dl", "pthread"])
        if self.options.with_cuda:
            self.cpp_info.defines.append("WITH_CUDA")