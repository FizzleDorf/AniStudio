cmake_minimum_required(VERSION 3.12)
project(AniPlugins VERSION 1.0.0 LANGUAGES CXX)

# Build options
option(ANIPLUGINS_BUILD_EXAMPLES "Build AniPlugins examples" ON)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform specific settings
if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# This is a source-only library - the .cpp files get compiled into the main projects
# No separate library target needed

set(ANIPLUGINS_SOURCES
    PluginAPI.cpp
    PluginManager.cpp
)

set(ANIPLUGINS_HEADERS
    PluginAPI.hpp
    PluginManager.hpp
)

# Export source files for parent projects
set(ANIPLUGINS_SOURCES ${ANIPLUGINS_SOURCES} PARENT_SCOPE)
set(ANIPLUGINS_HEADERS ${ANIPLUGINS_HEADERS} PARENT_SCOPE)

# Utility function for building plugins easily
function(add_anistudio_plugin TARGET_NAME)
    cmake_parse_arguments(PLUGIN "" "" "SOURCES;LIBRARIES;INCLUDE_DIRS" ${ARGN})
    
    # Create shared library (plugins must be shared)
    add_library(${TARGET_NAME} SHARED ${PLUGIN_SOURCES})
    
    # Include directories
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}  # For PluginAPI.hpp
        ${PLUGIN_INCLUDE_DIRS}
    )
    
    # NO LINKING - plugins only use the API headers
    # Linking causes the DLL corruption issues
    
    # Plugin export definition
    target_compile_definitions(${TARGET_NAME} PRIVATE
        PLUGIN_API=__declspec\(dllexport\)
    )
    
    # Set output properties
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        # Output to plugins directory
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        # Force same directory for all configurations
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins
    )
    
    # Platform-specific linking for basic system libraries
    if(WIN32)
        target_link_libraries(${TARGET_NAME} PRIVATE kernel32)
    else()
        target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_DL_LIBS})
    endif()
    
    message(STATUS "Added AniStudio plugin: ${TARGET_NAME}")
endfunction()

# Build example plugin if requested
if(ANIPLUGINS_BUILD_EXAMPLES)
    # Example plugin
    add_anistudio_plugin(ExamplePlugin
        SOURCES ExamplePlugin.cpp
    )
    
    message(STATUS "Built example plugin")
endif()

# Install configuration (when building standalone)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)
    
    # Install headers
    install(FILES ${ANIPLUGINS_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aniplugins
    )
endif()

message(STATUS "AniPlugins configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Examples: ${ANIPLUGINS_BUILD_EXAMPLES}")
message(STATUS "  Source files exported to parent projects")