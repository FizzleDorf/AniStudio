cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)

project(AniStudio VERSION 1.0.0)

# Build options
option(BUILD_ANIENGINE "Build AniEngine library" ON)
option(BUILD_ANISTUDIO "Build AniStudio library" ON)
option(BUILD_ANIPLUGINS "Build AniPlugins library" ON)
option(BUILD_MAIN_APP "Build the main AniStudio application" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)
option(SD_CUDA "Enable CUDA support for Stable Diffusion" OFF)

# Project-wide settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(UNIX AND NOT APPLE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

# CUDA setup if enabled
if(SD_CUDA)
    enable_language(CUDA)
    set(CUDA_ARCHITECTURES "89" CACHE STRING "Override for RTX 4090" FORCE)
    add_definitions(-DSD_USE_CUDA)
    
    find_package(CUDAToolkit REQUIRED)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDA toolkit not found!")
    endif()
endif()

# Include Conan toolchain
include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(STATUS "Conan toolchain not found, installing dependencies...")
    
    find_program(CONAN_CMD conan)
    if(NOT CONAN_CMD)
        message(FATAL_ERROR "Conan not found! Please install it first: pip install conan")
    endif()
    
    execute_process(
        COMMAND ${CONAN_CMD} install "${CMAKE_SOURCE_DIR}" 
                --output-folder="${CMAKE_BINARY_DIR}" 
                --build=missing
        RESULT_VARIABLE CONAN_RESULT
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    
    if(NOT CONAN_RESULT EQUAL 0)
        message(FATAL_ERROR "Conan install failed!")
    endif()
    
    include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake)
endif()

# Set output directories - FIXED VERSION
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ADDED: Force single output directory for all configurations
# This prevents CMake from adding /Debug or /Release subdirectories
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# Also set for single-config generators (like Makefiles)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)

# Create necessary directories
set(PLUGINS_DIR "${CMAKE_BINARY_DIR}/plugins")
set(DATA_DIR "${CMAKE_BINARY_DIR}/data")
set(DEFAULTS_DIR "${DATA_DIR}/defaults")
set(SCRIPTS_DIR "${CMAKE_BINARY_DIR}/scripts")

file(MAKE_DIRECTORY ${PLUGINS_DIR})
file(MAKE_DIRECTORY ${DATA_DIR})
file(MAKE_DIRECTORY ${DEFAULTS_DIR})
file(MAKE_DIRECTORY ${SCRIPTS_DIR})

# Copy default files
if(EXISTS "${CMAKE_SOURCE_DIR}/data/defaults")
    file(GLOB_RECURSE DEFAULT_FILES "${CMAKE_SOURCE_DIR}/data/defaults/*")
    foreach(DEFAULT_FILE ${DEFAULT_FILES})
        file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/data/defaults" "${DEFAULT_FILE}")
        set(DEST_FILE "${DEFAULTS_DIR}/${REL_PATH}")
        
        if(NOT EXISTS "${DEST_FILE}")
            get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
            file(MAKE_DIRECTORY "${DEST_DIR}")
            file(COPY "${DEFAULT_FILE}" DESTINATION "${DEST_DIR}")
        endif()
    endforeach()
endif()

# Copy scripts
if(EXISTS "${CMAKE_SOURCE_DIR}/scripts")
    file(GLOB_RECURSE SCRIPT_FILES "${CMAKE_SOURCE_DIR}/scripts/*")
    foreach(SCRIPT_FILE ${SCRIPT_FILES})
        file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/scripts" "${SCRIPT_FILE}")
        set(DEST_FILE "${SCRIPTS_DIR}/${REL_PATH}")
        
        get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_DIR}")
        file(COPY "${SCRIPT_FILE}" DESTINATION "${DEST_DIR}")
    endforeach()
endif()

# Copy shaders
if(EXISTS "${CMAKE_SOURCE_DIR}/src/shaders")
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
    file(COPY ${SHADER_DIR}/basic.vert ${SHADER_DIR}/basic.frag DESTINATION ${CMAKE_BINARY_DIR}/shaders)
    message(STATUS "Copied shaders to ${CMAKE_BINARY_DIR}/shaders")
endif()

# Find external dependencies (find_package only - no building here)
find_package(Vulkan REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(pybind11 REQUIRED)
find_package(semver REQUIRED)

# Check for Zep availability (but don't build it here)
if(EXISTS "${CMAKE_SOURCE_DIR}/external/zep/CMakeLists.txt")
    set(ZEP_AVAILABLE TRUE)
    message(STATUS "Zep text editor source found")
else()
    message(WARNING "Zep not found at external/zep - text editor features will be limited")
    set(ZEP_AVAILABLE FALSE)
endif()

# External libraries will be built by the individual library CMakeLists
# This avoids rebuilding them when only one library changes

# Pass options to subdirectories
set(ANIENGINE_BUILD_SHARED ${BUILD_SHARED_LIBS})
set(ANIENGINE_ENABLE_CUDA ${SD_CUDA})
set(ANISTUDIO_BUILD_SHARED ${BUILD_SHARED_LIBS})
set(ANISTUDIO_ENABLE_ZEP ${ZEP_AVAILABLE})
set(ANIPLUGINS_BUILD_SHARED ${BUILD_SHARED_LIBS})

# Add subdirectory libraries
if(BUILD_ANIPLUGINS)
    add_subdirectory(src/aniplugins)
endif()

if(BUILD_ANIENGINE)
    add_subdirectory(src/aniengine)
endif()

if(BUILD_ANISTUDIO)
    add_subdirectory(src/anistudio)
endif()

# Main application
if(BUILD_MAIN_APP AND BUILD_ANISTUDIO)
    add_executable(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/src/main.cpp
        ${CMAKE_SOURCE_DIR}/src/pch.cpp
        ${CMAKE_SOURCE_DIR}/src/core/Core.cpp
        ${CMAKE_SOURCE_DIR}/src/timer/Timer.cpp
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        # ADDED: Explicitly set output directories for main executable
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        GLEW_STATIC
        IMGUI_DEFINE_MATH_OPERATORS
    )
    
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/timer
        ${CMAKE_SOURCE_DIR}/include
    )
    
    # Link to AniStudioCore which includes AniEngineCore
    target_link_libraries(${PROJECT_NAME} PRIVATE
        AniStudioCore
    )

    # Precompiled headers
    target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
    
    message(STATUS "Main application will be built")
endif()

# Plugin utility function for the main project
function(add_cr_plugin TARGET_NAME)
    cmake_parse_arguments(PLUGIN "" "" "SOURCES;LIBRARIES;INCLUDE_DIRS" ${ARGN})
    
    add_library(${TARGET_NAME} SHARED ${PLUGIN_SOURCES})
    
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/aniplugins
        ${CMAKE_SOURCE_DIR}/external/cr/cr
        ${PLUGIN_INCLUDE_DIRS}
    )
    
    target_link_libraries(${TARGET_NAME} PRIVATE
        AniEngineCore
        ${PLUGIN_LIBRARIES}
    )
    
    target_compile_definitions(${TARGET_NAME} PRIVATE
        PLUGIN_API=__declspec\(dllexport\)
    )
    
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${PLUGINS_DIR}
        # ADDED: Force plugin output directories for all configurations
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PLUGINS_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PLUGINS_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PLUGINS_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${PLUGINS_DIR}
    )
    
    message(STATUS "Added CR plugin: ${TARGET_NAME}")
endfunction()

# Install configuration (when building full project)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)
    
    # Install the main application
    if(BUILD_MAIN_APP AND TARGET ${PROJECT_NAME})
        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
    
    # Install data and scripts
    install(DIRECTORY ${DATA_DIR}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
        PATTERN "*.tmp" EXCLUDE
    )
    
    install(DIRECTORY ${SCRIPTS_DIR}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scripts
        PATTERN "*.tmp" EXCLUDE
    )
    
    # Install shaders
    if(EXISTS ${CMAKE_BINARY_DIR}/shaders)
        install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/shaders
        )
    endif()
    
    # Create plugins directory
    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/plugins)
endif()

# Summary
message(STATUS "")
message(STATUS "AniStudio build configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  CUDA support: ${SD_CUDA}")
message(STATUS "")
message(STATUS "Components:")
message(STATUS "  AniPlugins: ${BUILD_ANIPLUGINS}")
message(STATUS "  AniEngine: ${BUILD_ANIENGINE}")
message(STATUS "  AniStudio: ${BUILD_ANISTUDIO}")
message(STATUS "  Main App: ${BUILD_MAIN_APP}")
message(STATUS "  Zep Editor: ${ZEP_AVAILABLE}")
message(STATUS "")
message(STATUS "Output directories:")
message(STATUS "  Binaries: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Libraries: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "  Plugins: ${PLUGINS_DIR}")
message(STATUS "")