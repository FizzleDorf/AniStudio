from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
import os

class AniStudio(ConanFile):
    name = "AniStudio"
    version = "0.1.0"
    
    # Binary configuration
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

    # Build requirements
    tool_requires = (
        "cmake/3.25.1",
    )

    def requirements(self):
        # Core dependencies from your conanfile.txt
        self.requires("opencv/4.5.5")
        self.requires("glfw/3.4")
        self.requires("glew/2.1.0")
        self.requires("zlib/1.2.11")
        self.requires("libpng/1.6.44")
        self.requires("exiv2/0.28.1")
        self.requires("ffmpeg/4.4.4")

        # Platform-specific OpenGL requirements
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

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = os.path.join(self.folders.build, "conan")

    def generate(self):
        # Generate CMake toolchain
        tc = CMakeToolchain(self)
        
        # Add custom definitions
        tc.variables["WITH_CUDA"] = self.options.with_cuda
            
        tc.generate()
        
        # Generate CMake dependency files
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        
        # Copy license files
        copy(self, "LICENSE*", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        
    def package_info(self):
        self.cpp_info.libs = ["media_creation_tool"]
        
        # Add any required system libraries
        if self.settings.os == "Linux":
            self.cpp_info.system_libs.extend(["dl", "pthread"])
        
        # Set any required defines
        if self.options.with_cuda:
            self.cpp_info.defines.append("WITH_CUDA")