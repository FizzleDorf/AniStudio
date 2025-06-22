cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)
project(ExamplePlugin VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# FIGURE OUT THE FUCKING PATHS OURSELVES
get_filename_component(PLUGIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
get_filename_component(ANISTUDIO_ROOT "${PLUGIN_SOURCE_DIR}/../.." ABSOLUTE)
set(ANISTUDIO_BUILD_DIR "${ANISTUDIO_ROOT}/build")
set(ANISTUDIO_PLUGIN_DIR "${CMAKE_CURRENT_BINARY_DIR}")

message(STATUS "ExamplePlugin configuration:")
message(STATUS "  Plugin source: ${PLUGIN_SOURCE_DIR}")
message(STATUS "  AniStudio root: ${ANISTUDIO_ROOT}")
message(STATUS "  AniStudio build dir: ${ANISTUDIO_BUILD_DIR}")
message(STATUS "  Plugin output dir: ${ANISTUDIO_PLUGIN_DIR}")

# Include Conan toolchain from AniStudio build directory
set(CONAN_TOOLCHAIN_FILE "${ANISTUDIO_BUILD_DIR}/conan/conan_toolchain.cmake")
if(EXISTS "${CONAN_TOOLCHAIN_FILE}")
    include("${CONAN_TOOLCHAIN_FILE}")
    message(STATUS "Found and included Conan toolchain: ${CONAN_TOOLCHAIN_FILE}")
else()
    message(FATAL_ERROR "Conan toolchain not found at: ${CONAN_TOOLCHAIN_FILE}")
endif()

# Include the AniStudio plugin helper
set(ANISTUDIO_PLUGIN_CMAKE "${ANISTUDIO_BUILD_DIR}/../cmake/AniStudioPlugin.cmake")
if(EXISTS "${ANISTUDIO_PLUGIN_CMAKE}")
    include("${ANISTUDIO_PLUGIN_CMAKE}")
    message(STATUS "Using AniStudio plugin helper: ${ANISTUDIO_PLUGIN_CMAKE}")
else()
    message(FATAL_ERROR "AniStudioPlugin.cmake not found at: ${ANISTUDIO_PLUGIN_CMAKE}")
endif()

# Create the plugin as a shared library
add_library(${PROJECT_NAME} SHARED
    ExamplePlugin.cpp
)

# Use the AniStudio plugin setup function to configure everything
setup_anistudio_plugin(${PROJECT_NAME})

# Add missing ImGui include directory that the helper forgot
target_include_directories(${PROJECT_NAME} PRIVATE
    ${ANISTUDIO_BUILD_DIR}/../external/imgui
    ${ANISTUDIO_BUILD_DIR}/../external/imgui/backends
)

# Link to required libraries that the helper is missing
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${ANISTUDIO_CORE_LIB}
    ${IMGUI_LIBRARY}
)

# Find and link ImGui library manually since the helper failed
find_library(IMGUI_LIB
    NAMES ImGui
    PATHS ${ANISTUDIO_BUILD_DIR}/lib
    NO_DEFAULT_PATH
)

if(IMGUI_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${IMGUI_LIB})
    message(STATUS "Linked ImGui: ${IMGUI_LIB}")
else()
    message(FATAL_ERROR "ImGui library not found in ${ANISTUDIO_BUILD_DIR}/lib")
endif()

# OVERRIDE: Disable PCH completely for plugins to avoid include path issues
set_target_properties(${PROJECT_NAME} PROPERTIES
    DISABLE_PRECOMPILE_HEADERS ON
)

# Clear any PCH that might have been set by the helper function
set_property(TARGET ${PROJECT_NAME} PROPERTY PRECOMPILE_HEADERS "")

# Copy the plugin manifest to the build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
    ${ANISTUDIO_PLUGIN_DIR}/plugin.json
    COPYONLY
)

message(STATUS "ExamplePlugin configuration complete")
message(STATUS "Plugin will be built to: ${ANISTUDIO_PLUGIN_DIR}")