cmake_minimum_required(VERSION 3.12)
project(AniPlugins VERSION 1.0.0 LANGUAGES CXX)

# Build options
option(ANIPLUGINS_BUILD_SHARED "Build AniPlugins as shared library" OFF)
option(ANIPLUGINS_BUILD_EXAMPLES "Build AniPlugins examples" OFF)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform specific settings
if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Find dependencies if building standalone
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - find AniEngine
    find_package(AniEngine QUIET)
    if(NOT AniEngine_FOUND)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../aniengine")
            message(STATUS "AniEngine not found, building from source")
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../aniengine ${CMAKE_BINARY_DIR}/aniengine)
        else()
            message(FATAL_ERROR "AniEngine not found and source not available")
        endif()
    endif()
endif()

# Collect source files
file(GLOB_RECURSE ANIPLUGINS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Remove any main.cpp or example files
list(FILTER ANIPLUGINS_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER ANIPLUGINS_SOURCES EXCLUDE REGEX ".*/examples/.*")

# Create the library (header-only by default, but can include base implementations)
if(ANIPLUGINS_SOURCES)
    if(ANIPLUGINS_BUILD_SHARED)
        add_library(AniPlugins SHARED ${ANIPLUGINS_SOURCES})
        target_compile_definitions(AniPlugins PRIVATE ANI_PLUGINS_EXPORTS)
        target_compile_definitions(AniPlugins PUBLIC ANI_PLUGINS_API=__declspec\(dllimport\))
    else()
        add_library(AniPlugins STATIC ${ANIPLUGINS_SOURCES})
        target_compile_definitions(AniPlugins PUBLIC ANI_PLUGINS_API=)
    endif()
    
    # Set library properties
    set_target_properties(AniPlugins PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    
    # Link to AniEngineCore
    target_link_libraries(AniPlugins PUBLIC AniEngineCore)
    
    set(ANIPLUGINS_TARGET AniPlugins)
else()
    # Header-only interface library
    add_library(AniPlugins INTERFACE)
    target_compile_definitions(AniPlugins INTERFACE ANI_PLUGINS_API=)
    
    # Link to AniEngineCore
    target_link_libraries(AniPlugins INTERFACE AniEngineCore)
    
    set(ANIPLUGINS_TARGET AniPlugins)
endif()

# Include directories
if(ANIPLUGINS_SOURCES)
    target_include_directories(${ANIPLUGINS_TARGET} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/aniplugins>
    )
else()
    target_include_directories(${ANIPLUGINS_TARGET} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/aniplugins>
    )
endif()

# Add root include directory for public headers
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - include directory is at ../../include
    if(ANIPLUGINS_SOURCES)
        target_include_directories(${ANIPLUGINS_TARGET} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../../include>
        )
    else()
        target_include_directories(${ANIPLUGINS_TARGET} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../../include>
        )
    endif()
else()
    # Parent project build - include directory is at root/include
    if(ANIPLUGINS_SOURCES)
        target_include_directories(${ANIPLUGINS_TARGET} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        )
    else()
        target_include_directories(${ANIPLUGINS_TARGET} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        )
    endif()
endif()

# External includes (when building standalone)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    if(ANIPLUGINS_SOURCES)
        target_include_directories(${ANIPLUGINS_TARGET} PUBLIC
            ${CMAKE_SOURCE_DIR}/../../external/cr/cr
        )
    else()
        target_include_directories(${ANIPLUGINS_TARGET} INTERFACE
            ${CMAKE_SOURCE_DIR}/../../external/cr/cr
        )
    endif()
else()
    # Parent project includes
    if(ANIPLUGINS_SOURCES)
        target_include_directories(${ANIPLUGINS_TARGET} PUBLIC
            ${CMAKE_SOURCE_DIR}/external/cr/cr
        )
    else()
        target_include_directories(${ANIPLUGINS_TARGET} INTERFACE
            ${CMAKE_SOURCE_DIR}/external/cr/cr
        )
    endif()
endif()

# Compile definitions
if(ANIPLUGINS_SOURCES)
    target_compile_definitions(${ANIPLUGINS_TARGET} PUBLIC
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        PLUGIN_API=__declspec\(dllexport\)  # For plugin DLLs
    )
else()
    target_compile_definitions(${ANIPLUGINS_TARGET} INTERFACE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        PLUGIN_API=__declspec\(dllexport\)  # For plugin DLLs
    )
endif()

# Install configuration
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)
    
    # Install the library
    install(TARGETS ${ANIPLUGINS_TARGET}
        EXPORT AniPluginsTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aniplugins
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
        PATTERN "examples" EXCLUDE
    )
    
    # Install public headers from include directory
    if(EXISTS ${CMAKE_SOURCE_DIR}/../../include)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/../../include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
        )
    endif()
    
    # Create and install config files
    include(CMakePackageConfigHelpers)
    
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/AniPluginsConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/AniPluginsConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniPlugins
    )
    
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/AniPluginsConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/AniPluginsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/AniPluginsConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniPlugins
    )
    
    install(EXPORT AniPluginsTargets
        FILE AniPluginsTargets.cmake
        NAMESPACE AniPlugins::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniPlugins
    )
endif()

# Utility function for building plugins
function(add_plugin_library TARGET_NAME)
    cmake_parse_arguments(PLUGIN "" "" "SOURCES;LIBRARIES;INCLUDE_DIRS" ${ARGN})
    
    add_library(${TARGET_NAME} SHARED ${PLUGIN_SOURCES})
    
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PLUGIN_INCLUDE_DIRS}
    )
    
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${ANIPLUGINS_TARGET}
        ${PLUGIN_LIBRARIES}
    )
    
    target_compile_definitions(${TARGET_NAME} PRIVATE
        PLUGIN_API=__declspec\(dllexport\)
    )
    
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    )
    
    message(STATUS "Added plugin: ${TARGET_NAME}")
endfunction()

# Examples (optional)
if(ANIPLUGINS_BUILD_EXAMPLES AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
        add_subdirectory(examples)
    endif()
endif()

message(STATUS "AniPlugins configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Has sources: ${ANIPLUGINS_SOURCES}")
message(STATUS "  Examples: ${ANIPLUGINS_BUILD_EXAMPLES}")