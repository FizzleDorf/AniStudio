cmake_minimum_required(VERSION 3.12)
project(AniEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Get paths
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../../external" ABSOLUTE)
    get_filename_component(ROOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../include" ABSOLUTE)
else()
    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external" ABSOLUTE)
    get_filename_component(ROOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include" ABSOLUTE)
endif()

# Collect sources
file(GLOB_RECURSE ANIENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/asset_mgr/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AniEngine.cpp"
)

# Create library - uses BUILD_SHARED_LIBS automatically
add_library(AniEngineCore ${ANIENGINE_SOURCES})

# CRITICAL FIX: Export symbols when building as DLL
if(BUILD_SHARED_LIBS)
    target_compile_definitions(AniEngineCore PRIVATE BUILDING_ANIENGINE)
    # On Windows, export all symbols automatically
    if(WIN32)
        set_target_properties(AniEngineCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
else()
    target_compile_definitions(AniEngineCore PRIVATE BUILDING_ANISTUDIO)
endif()

# Includes
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
    $<BUILD_INTERFACE:${ROOT_INCLUDE_DIR}>
    "${EXTERNAL_DIR}/glm"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/cr/cr"
    "${EXTERNAL_DIR}/imgui_filedialog/stb"
    "${EXTERNAL_DIR}/stb_image"
)

# Link
target_link_libraries(AniEngineCore PUBLIC
    ${OpenCV_LIBS}
    ZLIB::ZLIB
    PNG::PNG
    GLEW::GLEW
    glfw
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
    pybind11::embed
)

target_compile_definitions(AniEngineCore PUBLIC
    GLEW_STATIC
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

add_library(AniEngine::AniEngineCore ALIAS AniEngineCore)

message(STATUS "AniEngine: BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "AniEngine: Building as SHARED library (.dll) with symbol export")
else()
    message(STATUS "AniEngine: Building as STATIC library (.lib)")
endif()