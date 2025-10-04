cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)
project(AniEngine VERSION 1.0.0 LANGUAGES CXX)

# Use dynamic runtime to match parent project
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Build options
option(ANIENGINE_BUILD_EXAMPLES "Build AniEngine examples" OFF)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform specific settings
if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Use relative paths for external dependencies
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL)

    find_package(OpenCV REQUIRED)
    find_package(glew REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    find_package(FFmpeg REQUIRED)
    find_package(pybind11 REQUIRED)
    # find_package(semver REQUIRED)

    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../../external" ABSOLUTE)
else()
    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external" ABSOLUTE)
endif()

file(TO_CMAKE_PATH "${EXTERNAL_DIR}" EXTERNAL_DIR)

# Collect source files including plugin system
file(GLOB_RECURSE ANIENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asset_mgr/*.cpp"
)

# Add the main AniEngine implementation
list(APPEND ANIENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/AniEngine.cpp")

# Create the library
if(ANIENGINE_BUILD_SHARED)
    add_library(AniEngineCore SHARED ${ANIENGINE_SOURCES})
    target_compile_definitions(AniEngineCore PRIVATE ANI_ENGINE_EXPORTS)
    target_compile_definitions(AniEngineCore PUBLIC ANI_ENGINE_API=__declspec\(dllimport\))
else()
    add_library(AniEngineCore STATIC ${ANIENGINE_SOURCES})
    target_compile_definitions(AniEngineCore PUBLIC ANI_ENGINE_API=)
endif()

# Set library properties
set_target_properties(AniEngineCore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories using relative paths
target_include_directories(AniEngineCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asset_mgr>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/defaults>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/media>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/scripts>
    $<INSTALL_INTERFACE:include/aniengine>
)

# Add root include directory for public headers using relative paths
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    get_filename_component(ROOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../include" ABSOLUTE)
else()
    get_filename_component(ROOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include" ABSOLUTE)
endif()
target_include_directories(AniEngineCore PUBLIC
    $<BUILD_INTERFACE:${ROOT_INCLUDE_DIR}>
)

# External includes using relative paths
target_include_directories(AniEngineCore PUBLIC
    "${EXTERNAL_DIR}/glm"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/cr/cr"
	"${EXTERNAL_DIR}/imgui_filedialog/stb"
	"${EXTERNAL_DIR}/stb_image"
)

# Link base libraries
target_link_libraries(AniEngineCore PUBLIC
    ${OpenCV_LIBS}
    ZLIB::ZLIB
    PNG::PNG
    GLEW::GLEW
    glfw
    # PkgConfig::LIBAV
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
    pybind11::embed
    # semver::semver
)

# Compile definitions
target_compile_definitions(AniEngineCore PUBLIC
    GLEW_STATIC
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Create an alias for consistent naming
add_library(AniEngine::AniEngineCore ALIAS AniEngineCore)

# Debug: Print all backend targets that were found
message(STATUS "")
message(STATUS "AniEngine Backend Linking Summary:")
get_directory_property(ALL_TARGETS BUILDSYSTEM_TARGETS)

message(STATUS "")
message(STATUS "AniEngine configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Runtime library: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
message(STATUS "  External directory: ${EXTERNAL_DIR}")
message(STATUS "")