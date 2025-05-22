cmake_minimum_required(VERSION 3.12)

# Define the plugin name
set(PLUGIN_NAME "GpuMonitor")

# Get all source files - make sure paths exist
file(GLOB_RECURSE PLUGIN_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

# Fallback if no sources found - check actual locations
if(NOT PLUGIN_SOURCES)
    message(STATUS "No sources found in default locations. Checking for alternative structure...")
    file(GLOB_RECURSE PLUGIN_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    )
endif()

# Ensure we have sources
if(NOT PLUGIN_SOURCES)
    message(FATAL_ERROR "No source files found for plugin ${PLUGIN_NAME}. Please check directory structure.")
endif()

message(STATUS "Building plugin ${PLUGIN_NAME} with sources: ${PLUGIN_SOURCES}")

# Create the plugin shared library
add_library(${PLUGIN_NAME} MODULE ${PLUGIN_SOURCES})

# Set output directory
set_target_properties(${PLUGIN_NAME} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${PLUGINS_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${PLUGINS_DIR}"
)

# Include directories - adjust based on your actual structure
target_include_directories(${PLUGIN_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link against AniStudioCore and required libraries
target_link_libraries(${PLUGIN_NAME} PRIVATE
    AniStudioCore
    GLEW::GLEW
    glfw
)

# Define export symbol
if(WIN32)
    target_compile_definitions(${PLUGIN_NAME} PRIVATE PLUGIN_API=__declspec\(dllexport\))
else()
    target_compile_definitions(${PLUGIN_NAME} PRIVATE PLUGIN_API=)
endif()