cmake_minimum_required(VERSION 3.8)

# Set CMake policy CMP0079 to NEW
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

project(AniStudio)

# Set the directory containing the libraries
set(LIBRARY_DIR "${CMAKE_SOURCE_DIR}/external/libs")

set(CMAKE_GENERATOR_PLATFORM x64)

# Include the subdirectory for AniStudio
add_subdirectory(AniStudio)

# Include external libraries configuration
include(${CMAKE_SOURCE_DIR}/cmake/ExternalLibraries.cmake)
