cmake_minimum_required(VERSION 3.15)
project(AniStudio)

# Include Conan's CMake integration
include(${CMAKE_SOURCE_DIR}/build/conan_toolchain.cmake)
include(${CMAKE_SOURCE_DIR}/build/conandeps_legacy.cmake)

# Add your executable
add_executable(AniStudio ${CMAKE_SOURCE_DIR}/AniStudio/main.cpp)

# Link Conan dependencies
target_link_libraries(AniStudio PRIVATE
    CONAN_PKG::glfw
    CONAN_PKG::glew
    CONAN_PKG::imgui
    CONAN_PKG::sdl
)

# Find other required packages (if any)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(AniStudio PRIVATE
    GLEW::GLEW
    SDL2::SDL2
    OpenGL::GL
)
