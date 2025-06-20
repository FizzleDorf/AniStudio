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

if(UNIX AND NOT APPLE)
    set(OpenGL_GL_PREFERENCE GLVND)
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

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

# Create directories
set(PLUGINS_DIR "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGINS_DIR})

set(DATA_DIR "${CMAKE_BINARY_DIR}/data")
set(DEFAULTS_DIR "${DATA_DIR}/defaults")
file(MAKE_DIRECTORY ${DATA_DIR})
file(MAKE_DIRECTORY ${DEFAULTS_DIR})

file(GLOB_RECURSE DEFAULT_FILES "${CMAKE_SOURCE_DIR}/data/defaults/*")
foreach(DEFAULT_FILE ${DEFAULT_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/data/defaults" "${DEFAULT_FILE}")
    set(DEST_FILE "${DEFAULTS_DIR}/${REL_PATH}")
    
    if(NOT EXISTS "${DEST_FILE}")
        get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_DIR}")
        file(COPY "${DEFAULT_FILE}" DESTINATION "${DEST_DIR}")
        message(STATUS "Copied default file: ${REL_PATH}")
    else()
        message(STATUS "Skipping existing file: ${REL_PATH}")
    endif()
endforeach()

set(SCRIPTS_DIR "${CMAKE_BINARY_DIR}/scripts")
file(MAKE_DIRECTORY ${SCRIPTS_DIR})

if(EXISTS "${CMAKE_SOURCE_DIR}/scripts")
    file(GLOB_RECURSE SCRIPT_FILES "${CMAKE_SOURCE_DIR}/scripts/*")
    foreach(SCRIPT_FILE ${SCRIPT_FILES})
        file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/scripts" "${SCRIPT_FILE}")
        set(DEST_FILE "${SCRIPTS_DIR}/${REL_PATH}")
        
        get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_DIR}")
        file(COPY "${SCRIPT_FILE}" DESTINATION "${DEST_DIR}")
    endforeach()
    message(STATUS "Copied scripts directory to ${SCRIPTS_DIR}")
else()
    message(STATUS "No scripts directory found at ${CMAKE_SOURCE_DIR}/scripts")
endif()

# Find required packages
find_package(Vulkan REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(pybind11 REQUIRED)

include_directories(${GLFW_INCLUDE_DIRS})

# Shader files
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(COPY ${SHADER_DIR}/basic.vert ${SHADER_DIR}/basic.frag DESTINATION ${CMAKE_BINARY_DIR}/shaders)
message(STATUS "Copied shaders to ${CMAKE_BINARY_DIR}/shaders")

# Add external dependencies
add_subdirectory(external/stable-diffusion.cpp)
add_subdirectory(external/imgui)
add_subdirectory(external/imgui_nodes)
add_subdirectory(external/ImNodeFlow)
add_subdirectory(external/imgui_filedialog)
add_subdirectory(external/ImJSchema)
add_subdirectory(external/ImGuizmo)

# Only build Zep if needed
set(BUILD_IMGUI OFF CACHE BOOL "Build ImGui Demo" FORCE)
set(BUILD_QT OFF CACHE BOOL "Build Qt Demo" FORCE) 
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(ZEP_FEATURE_CPP_FILE_SYSTEM ON CACHE BOOL "Enable cpp file system" FORCE)

if(EXISTS "${CMAKE_SOURCE_DIR}/external/zep/CMakeLists.txt")
    add_subdirectory(external/zep)
    set(ZEP_AVAILABLE TRUE)
    message(STATUS "Zep text editor found and will be built")
else()
    message(WARNING "Zep not found at external/zep - text editor features will be limited")
    set(ZEP_AVAILABLE FALSE)
endif()

# Include directories
include_directories(
    ${CONAN_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/base_plugins
    ${CMAKE_SOURCE_DIR}/src/events
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/base
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/gui_utils
    ${CMAKE_SOURCE_DIR}/src/ecs
    ${CMAKE_SOURCE_DIR}/src/ecs/base
    ${CMAKE_SOURCE_DIR}/src/ecs/systems
    ${CMAKE_SOURCE_DIR}/src/ecs/components
    ${CMAKE_SOURCE_DIR}/src/ecs/components/SDCPPComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/ImageComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/VideoComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/ScriptComponents
    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
    ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes
    ${CMAKE_SOURCE_DIR}/external/immvision
    ${CMAKE_SOURCE_DIR}/external/imgui_markdown
    ${CMAKE_SOURCE_DIR}/external/zep/include
    ${Vulkan_INCLUDE_DIR}
)

# Source files
file(GLOB_RECURSE GUI_SOURCES ${CMAKE_SOURCE_DIR}/src/gui/*.cpp)
file(GLOB_RECURSE SYSTEM_SOURCES ${CMAKE_SOURCE_DIR}/src/ecs/systems/*.cpp)
file(GLOB_RECURSE UTILS_SOURCES ${CMAKE_SOURCE_DIR}/src/utils/*.cpp)
file(GLOB_RECURSE PLUGIN_SOURCES ${CMAKE_SOURCE_DIR}/src/base_plugins/*.cpp)
file(GLOB_RECURSE EVENT_SOURCES ${CMAKE_SOURCE_DIR}/src/events/*.cpp)
file(GLOB_RECURSE GUI_UTILS_SOURCES ${CMAKE_SOURCE_DIR}/src/gui_utils/*.cpp)

if(BUILD_ANISTUDIO)
    add_executable(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/src/main.cpp
        ${CMAKE_SOURCE_DIR}/src/pch.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/Engine.cpp
        ${CMAKE_SOURCE_DIR}/src/timer/Timer.cpp
        ${GUI_SOURCES}
        ${SYSTEM_SOURCES}
        ${UTILS_SOURCES}
        ${PLUGIN_SOURCES}
        ${EVENT_SOURCES}
        ${GUI_UTILS_SOURCES}
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # SIMPLE OPENGL FIX
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        GLEW_STATIC
        IMGUI_DEFINE_MATH_OPERATORS
    )
    
    link_directories(${CONAN_LIB_DIRS})

    if(SD_CUDA)
        target_compile_definitions(${PROJECT_NAME} PRIVATE SD_USE_CUDA)
        target_link_libraries(${PROJECT_NAME} PRIVATE CUDA::cudart)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CONAN_LIBS}
        stable-diffusion
        ImGui
        ImGui_Nodes
        ImNodeFlow
        ImJSchema
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
        pybind11::embed
    )

    if(ZEP_AVAILABLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE Zep::Zep)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ZEP_AVAILABLE)
    endif()

    target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE "ANI_CORE_API=__declspec(dllexport)")
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE "ANI_CORE_API=")
    endif()
endif()

# AniStudioCore library
add_library(AniStudioCore STATIC
    ${CMAKE_SOURCE_DIR}/src/pch.cpp
    ${GUI_SOURCES}
    ${SYSTEM_SOURCES}
    ${UTILS_SOURCES}
    ${PLUGIN_SOURCES}
    ${EVENT_SOURCES}
    ${GUI_UTILS_SOURCES}
)

# SIMPLE OPENGL FIX FOR CORE TOO
target_compile_definitions(AniStudioCore PUBLIC 
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
)

target_include_directories(AniStudioCore PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/base_plugins
    ${CMAKE_SOURCE_DIR}/src/events
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/base
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/gui_utils
    ${CMAKE_SOURCE_DIR}/src/ecs
    ${CMAKE_SOURCE_DIR}/src/ecs/base
    ${CMAKE_SOURCE_DIR}/src/ecs/systems
    ${CMAKE_SOURCE_DIR}/src/ecs/components
    ${CMAKE_SOURCE_DIR}/src/ecs/components/SDCPPComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/ImageComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/VideoComponents
    ${CMAKE_SOURCE_DIR}/src/ecs/components/ScriptComponents
    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
    ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes
    ${CMAKE_SOURCE_DIR}/external/immvision
    ${CMAKE_SOURCE_DIR}/external/imgui_markdown
    ${CMAKE_SOURCE_DIR}/external/zep/include
)

target_link_libraries(AniStudioCore PRIVATE
    ${CONAN_LIBS}
    stable-diffusion
    ImGui
    ImGui_Nodes
    ImNodeFlow
    ImJSchema
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
    pybind11::embed
)

if(ZEP_AVAILABLE)
    target_link_libraries(AniStudioCore PRIVATE Zep::Zep)
    target_compile_definitions(AniStudioCore PUBLIC ZEP_AVAILABLE)
endif()

if(SD_CUDA)
    target_compile_definitions(AniStudioCore PUBLIC SD_USE_CUDA)
    target_link_libraries(AniStudioCore PRIVATE CUDA::cudart)
endif()

if(WIN32)
    target_compile_definitions(AniStudioCore PUBLIC "ANI_CORE_API=" "PLUGIN_API=__declspec(dllexport)")
else()
    target_compile_definitions(AniStudioCore PUBLIC "ANI_CORE_API=" "PLUGIN_API=__attribute__((visibility(\"default\")))")
endif()

set_target_properties(AniStudioCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib
)

target_compile_features(AniStudioCore PUBLIC cxx_std_17)

message(STATUS "AniStudio build configuration complete")