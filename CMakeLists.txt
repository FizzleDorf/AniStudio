cmake_minimum_required(VERSION 3.8)

project(AniStudio)

# Set CMake policy CMP0079 to NEW
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

# Add options for backend selection
option(USE_GLFW "Use GLFW for windowing" ON)
option(USE_OPENGL "Use OpenGL for main rendering" ON)

# Set the directory containing the external libraries
add_subdirectory(external)

# Set the directory containing the AniStudio source files
add_subdirectory(AniStudio)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
