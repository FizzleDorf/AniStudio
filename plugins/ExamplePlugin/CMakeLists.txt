cmake_minimum_required(VERSION 3.20)
project(ExamplePlugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Require path to AniStudio
if(NOT DEFINED ANISTUDIO_ROOT)
    message(FATAL_ERROR "ANISTUDIO_ROOT must be defined. Use -DANISTUDIO_ROOT=/path/to/anistudio")
endif()

# Include Conan-generated cmake files from AniStudio's build directory
include(${ANISTUDIO_ROOT}/build/generators/conan_toolchain.cmake)

# Find required packages
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)

# Create base support library
add_library(AniStudioBase SHARED
    ${ANISTUDIO_ROOT}/AniStudio/ECS/Base/EntityManager.cpp
    ${ANISTUDIO_ROOT}/AniStudio/GUI/Base/ViewManager.cpp
)

# Create shared library
add_library(${PROJECT_NAME} SHARED
    ExamplePlugin.cpp
    ExamplePlugin.hpp
    ExampleComponent.hpp
    ExampleSystem.hpp
    ExampleView.hpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${ANISTUDIO_ROOT}
    ${ANISTUDIO_ROOT}/AniStudio
    ${ANISTUDIO_ROOT}/AniStudio/ECS/Base
    ${ANISTUDIO_ROOT}/AniStudio/GUI/Base
    ${ANISTUDIO_ROOT}/external/glm
    ${ANISTUDIO_ROOT}/external/imgui
    ${ANISTUDIO_ROOT}/external/nlohmann_json
    ${CONAN_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ImGui
    GLEW::GLEW
    glfw
    AniStudioBase
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# Output settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    PREFIX ""  # Remove lib prefix on Unix
)