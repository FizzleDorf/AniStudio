cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)

project(AniStudio)

if(SD_CUDA)
    enable_language(CUDA)
    set(CUDA_ARCHITECTURES "89" CACHE STRING "Override for RTX 4090" FORCE)
    add_definitions(-DSD_USE_CUDA)
    
    # Find CUDA toolkit
    find_package(CUDAToolkit REQUIRED)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDA toolkit not found!")
    endif()
endif()

option(BUILD_ANISTUDIO "Build the AniStudio application" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Include Conan toolchain
include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(STATUS "Conan toolchain not found, installing dependencies...")
    
    find_program(CONAN_CMD conan)
    if(NOT CONAN_CMD)
        message(FATAL_ERROR "Conan not found! Please install it first: pip install conan")
    endif()
    
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

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

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Set binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)

# For libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

# For static libraries and import libraries on Windows
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

# Create plugins directory
set(PLUGINS_DIR "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGINS_DIR})

# Data directory setup
set(DATA_DIR "${CMAKE_BINARY_DIR}/data")
set(DEFAULTS_DIR "${DATA_DIR}/defaults")
file(MAKE_DIRECTORY ${DATA_DIR})
file(MAKE_DIRECTORY ${DEFAULTS_DIR})
file(COPY "${CMAKE_SOURCE_DIR}/data/defaults/" DESTINATION "${DEFAULTS_DIR}")
message(STATUS "Copied data/defaults to ${DEFAULTS_DIR}")

# Find required packages
find_package(Vulkan REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(FFmpeg REQUIRED)

include_directories(${GLFW_INCLUDE_DIRS})

# Shader files
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/AniStudio/shaders")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(COPY ${SHADER_DIR}/basic.vert ${SHADER_DIR}/basic.frag DESTINATION ${CMAKE_BINARY_DIR}/shaders)
message(STATUS "Copied shaders to ${CMAKE_BINARY_DIR}/shaders")

# Add external dependencies
add_subdirectory(external/stable-diffusion.cpp)
add_subdirectory(external/imgui)
add_subdirectory(external/imgui_nodes)
add_subdirectory(external/ImNodeFlow)
# add_subdirectory(external/stb_image)
add_subdirectory(external/imgui_filedialog)
add_subdirectory(external/ImJSchema)
add_subdirectory(external/ImGuizmo)

set(BUILD_IMGUI OFF CACHE BOOL "Build ImGui Demo" FORCE)
set(BUILD_QT OFF CACHE BOOL "Build Qt Demo" FORCE) 
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(ZEP_FEATURE_CPP_FILE_SYSTEM ON CACHE BOOL "Enable cpp file system" FORCE)

add_subdirectory(external/zep)

# For external/imgui directory
target_include_directories(ImGui PRIVATE ${GLFW_INCLUDE_DIRS})
add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

# Common include directories
set(ANISTUDIO_INCLUDE_DIRS
    ${CONAN_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/AniStudio
    ${CMAKE_SOURCE_DIR}/AniStudio/Plugins
    ${CMAKE_SOURCE_DIR}/AniStudio/Events
    ${CMAKE_SOURCE_DIR}/AniStudio/GUI
    ${CMAKE_SOURCE_DIR}/AniStudio/GUI/Base
    ${CMAKE_SOURCE_DIR}/AniStudio/utils
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Systems
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/SDCPPComponents
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/ImageComponents
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/VideoComponents
    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
    ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes
    ${CMAKE_SOURCE_DIR}/external/immvision
	${CMAKE_SOURCE_DIR}/external/zep/include
    ${Vulkan_INCLUDE_DIR}
)

# Add the main AniStudio directory first for PCH
include_directories(${CMAKE_SOURCE_DIR}/AniStudio)

# Common link libraries
set(ANISTUDIO_LINK_LIBS
    ${CONAN_LIBS}
    stable-diffusion
    ImGui
    ImGui_Nodes
    ImNodeFlow
    ImJSchema
    # stb_image
    ImGuiFileDialog
    ImGuizmo
    opencv::opencv
    glfw
    GLEW::GLEW
    ZLIB::ZLIB
    PNG::PNG
    Vulkan::Vulkan
    exiv2::exiv2
    ffmpeg::avcodec
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
	Zep::Zep
)

# ============================================================================
# AniStudioCore Static Library
# ============================================================================

# Collect all core source files (excluding main.cpp, Timer/, Engine/)
file(GLOB_RECURSE GUI_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/GUI/*.cpp)
file(GLOB_RECURSE SYSTEM_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Systems/*.cpp)
file(GLOB_RECURSE UTILS_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/utils/*.cpp)
file(GLOB_RECURSE PLUGIN_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/Plugins/*.cpp)
file(GLOB_RECURSE EVENT_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/Events/*.cpp)

# Create AniStudioCore static library
add_library(AniStudioCore STATIC
    ${CMAKE_SOURCE_DIR}/AniStudio/pch.cpp
    ${GUI_SOURCES}
    ${SYSTEM_SOURCES}
    ${UTILS_SOURCES}
    ${PLUGIN_SOURCES}
    ${EVENT_SOURCES}
)

# Create an alias for the namespaced target
add_library(AniStudioCore::AniStudioCore ALIAS AniStudioCore)

# Include directories for AniStudioCore
target_include_directories(AniStudioCore PUBLIC 
    $<BUILD_INTERFACE:${ANISTUDIO_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/external>
)

# Link libraries for AniStudioCore
target_link_libraries(AniStudioCore PUBLIC ${ANISTUDIO_LINK_LIBS})

# Add CUDA support if enabled
if(SD_CUDA)
    target_compile_definitions(AniStudioCore PUBLIC SD_USE_CUDA)
    target_link_libraries(AniStudioCore PUBLIC CUDA::cudart)
endif()

# Set library properties
set_target_properties(AniStudioCore PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib
    EXPORT_NAME AniStudioCore
)

# Precompiled headers for AniStudioCore - PUBLIC so plugins can use them too
target_precompile_headers(AniStudioCore PUBLIC AniStudio/pch.h)

# Export definitions for AniStudioCore
if(WIN32)
    target_compile_definitions(AniStudioCore PUBLIC 
        "ANI_CORE_EXPORTS"
        "ANI_CORE_API="
        "PLUGIN_API="
    )
else()
    target_compile_definitions(AniStudioCore PUBLIC 
        "ANI_CORE_API="
        "PLUGIN_API=__attribute__((visibility(\"default\")))"
    )
endif()

message(STATUS "AniStudioCore library configured")

# ============================================================================
# AniStudio Executable (only if BUILD_ANISTUDIO is ON)
# ============================================================================

if(BUILD_ANISTUDIO)
    # Main executable - only main.cpp, Engine/, and Timer/
    add_executable(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/AniStudio/main.cpp
        ${CMAKE_SOURCE_DIR}/AniStudio/Engine/Engine.cpp
        ${CMAKE_SOURCE_DIR}/AniStudio/Timer/Timer.cpp
    )

    # Include directories for main executable
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_SOURCE_DIR}/AniStudio/Engine
        ${CMAKE_SOURCE_DIR}/AniStudio/Timer
    )

    # Link to AniStudioCore (which provides everything else)
    target_link_libraries(${PROJECT_NAME} PRIVATE AniStudioCore)

    # PCH for main executable too
    target_precompile_headers(${PROJECT_NAME} REUSE_FROM AniStudioCore)

    # Set executable properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # Symbol export definitions for main executable
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE 
            "ANI_CORE_EXPORTS"
            "ANI_CORE_API=__declspec(dllexport)"
        )
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE 
            "ANI_CORE_API="
        )
    endif()

    message(STATUS "AniStudio executable configured")
endif()

# ============================================================================
# Install targets for plugin development
# ============================================================================

# Install AniStudioCore library and headers for plugin development
install(TARGETS AniStudioCore
    EXPORT AniStudioCoreTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install the export targets
install(EXPORT AniStudioCoreTargets
    FILE AniStudioCoreTargets.cmake
    NAMESPACE AniStudioCore::
    DESTINATION lib/cmake/AniStudioCore
)

# Install all public headers needed by plugins
install(DIRECTORY ${CMAKE_SOURCE_DIR}/AniStudio/
    DESTINATION include/AniStudio
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "private" EXCLUDE
    PATTERN "internal" EXCLUDE
)

# Install external headers needed by plugins
install(DIRECTORY ${CMAKE_SOURCE_DIR}/external/nlohmann_json/
    DESTINATION include/external/nlohmann_json
    FILES_MATCHING PATTERN "*.hpp"
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/external/imgui/
    DESTINATION include/external/imgui
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/external/glm/
    DESTINATION include/external/glm
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Install stable-diffusion.cpp headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp/
    DESTINATION include/external/stable-diffusion.cpp
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Create cmake directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake)

# Create a CMake config file for plugins to find AniStudioCore
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/AniStudioCoreConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/lib/cmake/AniStudioCore/AniStudioCoreConfig.cmake"
    INSTALL_DESTINATION lib/cmake/AniStudioCore
    PATH_VARS CMAKE_INSTALL_PREFIX
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/lib/cmake/AniStudioCore/AniStudioCoreConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_BINARY_DIR}/lib/cmake/AniStudioCore/AniStudioCoreConfig.cmake"
    "${CMAKE_BINARY_DIR}/lib/cmake/AniStudioCore/AniStudioCoreConfigVersion.cmake"
    DESTINATION lib/cmake/AniStudioCore
)

message(STATUS "AniStudio build configuration complete")
message(STATUS "After building, plugins can be built independently using the installed AniStudioCore library")