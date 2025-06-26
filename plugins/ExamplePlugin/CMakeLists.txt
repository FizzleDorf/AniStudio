# CMakeLists.txt for ExamplePlugin - Simple approach that works with YOUR system
cmake_minimum_required(VERSION 3.15)
project(ExamplePlugin VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific settings
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
endif()

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Auto-detect host paths
get_filename_component(ANISTUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(PLUGINS_OUTPUT_DIR "${ANISTUDIO_ROOT}/build/plugins")

# Include directories - minimal, only what we need
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANISTUDIO_ROOT}/external/imgui                      # ImGui headers only
)

# Plugin source files
set(PLUGIN_SOURCES
    ExamplePlugin.cpp
)

# Create the plugin as a shared library
add_library(${PROJECT_NAME} SHARED 
    ${PLUGIN_SOURCES}
)

# Platform-specific library naming and settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".dll"
        PREFIX ""
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".dylib"
        PREFIX "lib"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".so"
        PREFIX "lib"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Compiler flags
target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
)

# Link only system libraries - no complex host dependencies
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        user32
        kernel32
    )
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME})
else()
    target_link_libraries(${PROJECT_NAME}
        dl
        pthread
    )
endif()

# Create plugins output directory
if(NOT EXISTS ${PLUGINS_OUTPUT_DIR})
    file(MAKE_DIRECTORY ${PLUGINS_OUTPUT_DIR})
endif()

# Copy to plugins directory after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        $<TARGET_FILE:${PROJECT_NAME}> 
        ${PLUGINS_OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>
    COMMENT "Copying plugin to plugins directory"
)

# Print build information
message(STATUS "=== ${PROJECT_NAME} Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Host Directory: ${ANISTUDIO_ROOT}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Plugins Directory: ${PLUGINS_OUTPUT_DIR}")
message(STATUS "Using simple self-contained plugin approach")