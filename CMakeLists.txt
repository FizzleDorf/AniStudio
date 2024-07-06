cmake_minimum_required(VERSION 3.8)

project(AniStudio)

# Set CMake policy CMP0079 to NEW
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

set(CMAKE_GENERATOR_PLATFORM x64)

# Add options for backend selection
option(USE_GLFW "Use GLFW for windowing" ON)
option(USE_SDL2 "Use SDL2 for windowing" OFF)
option(USE_VULKAN "Use Vulkan for main rendering" ON)

# Set the directory containing the libraries
set(LIBRARY_DIR "${CMAKE_SOURCE_DIR}/external/libs")

# Add subdirectories
add_subdirectory(AniStudio)

# Include external libraries configuration
include(${CMAKE_SOURCE_DIR}/cmake/ExternalLibraries.cmake)
