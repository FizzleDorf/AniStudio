cmake_minimum_required(VERSION 3.8)

project(AniStudio)

# Set CMake policy CMP0079 to NEW
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

# Add options for backend selection
option(USE_GLFW "Use GLFW for windowing" ON)
option(USE_VULKAN "Use Vulkan for main rendering" OFF)
option(USE_OPENGL "Use OpenGL for main rendering" ON)

# Set the directory containing the libraries
set(LIBRARY_DIR "${CMAKE_SOURCE_DIR}/external/libs")

# Include the custom script for external libraries
include(${CMAKE_SOURCE_DIR}/cmake/ExternalLibraries.cmake)

# Add subdirectory for AniStudio
add_subdirectory(AniStudio)

# Example of setting C++ standard for the entire project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Example of setting output directory for executables
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Example of setting output directory for libraries
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
