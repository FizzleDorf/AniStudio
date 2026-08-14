cmake_minimum_required(VERSION 3.12)
project(AniEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../../external" ABSOLUTE)
    get_filename_component(ROOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../include" ABSOLUTE)
else()
    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external" ABSOLUTE)
    get_filename_component(ROOT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include" ABSOLUTE)
endif()

file(GLOB_RECURSE ANIENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(AniEngineCore ${ANIENGINE_SOURCES})

if(BUILD_SHARED_LIBS)
    target_compile_definitions(AniEngineCore PRIVATE BUILDING_ANIENGINE)
    if(WIN32)
        set_target_properties(AniEngineCore PROPERTIES 
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    else()
        set_target_properties(AniEngineCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            PREFIX ""
        )
    endif()
else()
    target_compile_definitions(AniEngineCore PRIVATE BUILDING_ANIENGINE)
endif()

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
    "${EXTERNAL_DIR}/stb_image"
)

target_link_libraries(AniEngineCore PUBLIC
    ZLIB::ZLIB
    PNG::PNG
    GLEW::GLEW
    glfw
    pybind11::embed
    libwebp::libwebp
    exiv2::exiv2
)

if(UNIX AND NOT APPLE)
    target_link_libraries(AniEngineCore PUBLIC
        pthread
        dl
        rt
    )
endif()

target_compile_definitions(AniEngineCore PUBLIC
    GLEW_STATIC
    USE_WEBP
    USE_EXIV2
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

add_library(AniEngine::AniEngineCore ALIAS AniEngineCore)

message(STATUS "AniEngine: BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "AniEngine: Building as SHARED library (.dll/.so)")
    message(STATUS "AniEngine: Output -> ${CMAKE_BINARY_DIR}/lib")
else()
    message(STATUS "AniEngine: Building as STATIC library (.lib/.a)")
    message(STATUS "AniEngine: Output -> ${CMAKE_BINARY_DIR}/lib")
endif()