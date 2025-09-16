cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)
project(AniEngine VERSION 1.0.0 LANGUAGES CXX)

# Use dynamic runtime to match parent project
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Build options
option(ANIENGINE_BUILD_SHARED "Build AniEngine as shared library" OFF)
option(ANIENGINE_ENABLE_CUDA "Enable CUDA support" OFF)
option(ANIENGINE_ENABLE_VULKAN "Enable Vulkan support" OFF)
option(ANIENGINE_ENABLE_HIPBLAS "Enable HipBLAS support for AMD GPUs" OFF)
option(ANIENGINE_ENABLE_METAL "Enable Metal support for Apple devices" OFF)
option(ANIENGINE_ENABLE_OPENCL "Enable OpenCL support" OFF)
option(ANIENGINE_ENABLE_SYCL "Enable SYCL support for Intel GPUs" OFF)
option(ANIENGINE_ENABLE_MUSA "Enable MUSA support for Moore Threads GPUs" OFF)
option(ANIENGINE_ENABLE_FAST_SOFTMAX "Enable fast softmax (may be less deterministic)" OFF)
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

# CRITICAL: Force stable-diffusion.cpp to use same runtime
set(SD_CUDA ${ANIENGINE_ENABLE_CUDA} CACHE BOOL "CUDA backend for stable-diffusion" FORCE)
set(SD_VULKAN ${ANIENGINE_ENABLE_VULKAN} CACHE BOOL "Vulkan backend for stable-diffusion" FORCE)
set(SD_HIPBLAS ${ANIENGINE_ENABLE_HIPBLAS} CACHE BOOL "HipBLAS backend for stable-diffusion" FORCE)
set(SD_METAL ${ANIENGINE_ENABLE_METAL} CACHE BOOL "Metal backend for stable-diffusion" FORCE)
set(SD_OPENCL ${ANIENGINE_ENABLE_OPENCL} CACHE BOOL "OpenCL backend for stable-diffusion" FORCE)
set(SD_SYCL ${ANIENGINE_ENABLE_SYCL} CACHE BOOL "SYCL backend for stable-diffusion" FORCE)
set(SD_MUSA ${ANIENGINE_ENABLE_MUSA} CACHE BOOL "MUSA backend for stable-diffusion" FORCE)
set(SD_FAST_SOFTMAX ${ANIENGINE_ENABLE_FAST_SOFTMAX} CACHE BOOL "Fast softmax for stable-diffusion" FORCE)

# CRITICAL: Also set GGML options to match
set(GGML_CUDA ${ANIENGINE_ENABLE_CUDA} CACHE BOOL "GGML CUDA" FORCE)
set(GGML_VULKAN ${ANIENGINE_ENABLE_VULKAN} CACHE BOOL "GGML Vulkan" FORCE)
set(GGML_HIPBLAS ${ANIENGINE_ENABLE_HIPBLAS} CACHE BOOL "GGML HipBLAS" FORCE)
set(GGML_METAL ${ANIENGINE_ENABLE_METAL} CACHE BOOL "GGML Metal" FORCE)
set(GGML_OPENCL ${ANIENGINE_ENABLE_OPENCL} CACHE BOOL "GGML OpenCL" FORCE)
set(GGML_SYCL ${ANIENGINE_ENABLE_SYCL} CACHE BOOL "GGML SYCL" FORCE)
set(GGML_MUSA ${ANIENGINE_ENABLE_MUSA} CACHE BOOL "GGML MUSA" FORCE)

# Backend setup and validation
if(ANIENGINE_ENABLE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES "native" CACHE STRING "CUDA architectures" FORCE)
    endif()
    
    add_definitions(-DSD_USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "AniEngine: CUDA backend enabled with architectures: ${CMAKE_CUDA_ARCHITECTURES}")
    
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDA toolkit not found!")
    endif()
endif()

if(ANIENGINE_ENABLE_VULKAN)
    add_definitions(-DSD_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan SDK not found! Install from https://www.lunarg.com/vulkan-sdk/")
    endif()
    message(STATUS "AniEngine: Vulkan backend enabled")
endif()

if(ANIENGINE_ENABLE_OPENCL)
    add_definitions(-DSD_USE_OPENCL)
    find_package(OpenCL REQUIRED)
    if(NOT OpenCL_FOUND)
        message(FATAL_ERROR "OpenCL not found - make sure OpenCL headers and ICD loader are installed")
    endif()
    message(STATUS "AniEngine: OpenCL backend enabled")
endif()

if(ANIENGINE_ENABLE_HIPBLAS)
    add_definitions(-DSD_USE_CUDA)
    message(STATUS "AniEngine: HipBLAS backend enabled")
endif()

if(ANIENGINE_ENABLE_METAL)
    add_definitions(-DSD_USE_METAL)
    message(STATUS "AniEngine: Metal backend enabled")
    if(NOT APPLE)
        message(WARNING "Metal backend is only supported on Apple platforms")
    endif()
endif()

if(ANIENGINE_ENABLE_SYCL)
    add_definitions(-DSD_USE_SYCL)
    message(STATUS "AniEngine: SYCL backend enabled")
endif()

if(ANIENGINE_ENABLE_MUSA)
    add_definitions(-DSD_USE_CUDA)
    message(STATUS "AniEngine: MUSA backend enabled")
endif()

# Use relative paths for external dependencies
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL)
    
    find_package(Vulkan REQUIRED)
    find_package(OpenCV REQUIRED)
    find_package(glew REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    find_package(FFmpeg REQUIRED)
    find_package(pybind11 REQUIRED)
    find_package(semver REQUIRED)
    
    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../../external" ABSOLUTE)
else()
    get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external" ABSOLUTE)
endif()

file(TO_CMAKE_PATH "${EXTERNAL_DIR}" EXTERNAL_DIR)

# Add stable-diffusion.cpp subdirectory - ONLY if not already added
if(NOT TARGET stable-diffusion AND EXISTS "${EXTERNAL_DIR}/stable-diffusion.cpp")
    message(STATUS "Adding stable-diffusion.cpp from: ${EXTERNAL_DIR}/stable-diffusion.cpp")
    add_subdirectory("${EXTERNAL_DIR}/stable-diffusion.cpp" "${CMAKE_BINARY_DIR}/stable-diffusion")
else()
    message(STATUS "stable-diffusion.cpp already available or not found at: ${EXTERNAL_DIR}/stable-diffusion.cpp")
endif()

# Collect source files including plugin system
file(GLOB_RECURSE ANIENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp"
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/SDCPPComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/ImageComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/VideoComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/ScriptComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/defaults>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/media>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/scripts>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/sdcpp>
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
    "${EXTERNAL_DIR}/stable-diffusion.cpp"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/cr/cr"
)

# Link base libraries
target_link_libraries(AniEngineCore PUBLIC
    opencv::opencv
    ZLIB::ZLIB
    PNG::PNG
    Vulkan::Vulkan
    GLEW::GLEW
    glfw
    ffmpeg::avcodec
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
    pybind11::embed
    semver::semver
    stable-diffusion
)

# CUDA support - COMPLETE LINKING
if(ANIENGINE_ENABLE_CUDA)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)
    target_link_libraries(AniEngineCore PUBLIC 
        CUDA::cudart
        CUDA::cublas
        CUDA::cublasLt
        CUDA::curand
        CUDA::cufft
    )
    
    # Link all possible CUDA backend targets
    foreach(CUDA_TARGET ggml-cuda ggml_cuda ggml-backend-cuda common-cuda)
        if(TARGET ${CUDA_TARGET})
            target_link_libraries(AniEngineCore PUBLIC ${CUDA_TARGET})
            message(STATUS "Linked CUDA backend: ${CUDA_TARGET}")
        endif()
    endforeach()
endif()

# Vulkan support - COMPLETE LINKING
if(ANIENGINE_ENABLE_VULKAN)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_VULKAN)
    target_link_libraries(AniEngineCore PUBLIC Vulkan::Vulkan)
    
    # Link all possible Vulkan backend targets
    foreach(VULKAN_TARGET ggml-vulkan ggml_vulkan ggml-backend-vulkan common-vulkan)
        if(TARGET ${VULKAN_TARGET})
            target_link_libraries(AniEngineCore PUBLIC ${VULKAN_TARGET})
            message(STATUS "Linked Vulkan backend: ${VULKAN_TARGET}")
        endif()
    endforeach()
endif()

# OpenCL support - COMPLETE LINKING
if(ANIENGINE_ENABLE_OPENCL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_OPENCL)
    target_link_libraries(AniEngineCore PUBLIC OpenCL::OpenCL)
    
    # Link all possible OpenCL backend targets
    foreach(OPENCL_TARGET ggml-opencl ggml_opencl ggml-backend-opencl common-opencl)
        if(TARGET ${OPENCL_TARGET})
            target_link_libraries(AniEngineCore PUBLIC ${OPENCL_TARGET})
            message(STATUS "Linked OpenCL backend: ${OPENCL_TARGET}")
        endif()
    endforeach()
endif()

# Metal support
if(ANIENGINE_ENABLE_METAL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_METAL)
    
    foreach(METAL_TARGET ggml-metal ggml_metal ggml-backend-metal common-metal)
        if(TARGET ${METAL_TARGET})
            target_link_libraries(AniEngineCore PUBLIC ${METAL_TARGET})
            message(STATUS "Linked Metal backend: ${METAL_TARGET}")
        endif()
    endforeach()
endif()

# HipBLAS support
if(ANIENGINE_ENABLE_HIPBLAS)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)
    
    foreach(HIPBLAS_TARGET ggml-hipblas ggml_hipblas ggml-backend-hipblas common-hipblas)
        if(TARGET ${HIPBLAS_TARGET})
            target_link_libraries(AniEngineCore PUBLIC ${HIPBLAS_TARGET})
            message(STATUS "Linked HipBLAS backend: ${HIPBLAS_TARGET}")
        endif()
    endforeach()
endif()

# SYCL support
if(ANIENGINE_ENABLE_SYCL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_SYCL)
    
    foreach(SYCL_TARGET ggml-sycl ggml_sycl ggml-backend-sycl common-sycl)
        if(TARGET ${SYCL_TARGET})
            target_link_libraries(AniEngineCore PUBLIC ${SYCL_TARGET})
            message(STATUS "Linked SYCL backend: ${SYCL_TARGET}")
        endif()
    endforeach()
endif()

# MUSA support
if(ANIENGINE_ENABLE_MUSA)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)
    
    foreach(MUSA_TARGET ggml-musa ggml_musa ggml-backend-musa common-musa)
        if(TARGET ${MUSA_TARGET})
            target_link_libraries(AniEngineCore PUBLIC ${MUSA_TARGET})
            message(STATUS "Linked MUSA backend: ${MUSA_TARGET}")
        endif()
    endforeach()
endif()

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
foreach(TARGET_NAME ${ALL_TARGETS})
    if(TARGET ${TARGET_NAME})
        if(${TARGET_NAME} MATCHES ".*ggml.*" OR ${TARGET_NAME} MATCHES ".*cuda.*" OR ${TARGET_NAME} MATCHES ".*vulkan.*" OR ${TARGET_NAME} MATCHES ".*opencl.*")
            get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
            if(TARGET_TYPE MATCHES "STATIC_LIBRARY|SHARED_LIBRARY")
                message(STATUS "  Available backend: ${TARGET_NAME} (${TARGET_TYPE})")
            endif()
        endif()
    endif()
endforeach()

message(STATUS "")
message(STATUS "AniEngine configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Runtime library: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
message(STATUS "  External directory: ${EXTERNAL_DIR}")
message(STATUS "")
message(STATUS "Stable Diffusion Backends:")
message(STATUS "  CUDA: ${ANIENGINE_ENABLE_CUDA}")
message(STATUS "  Vulkan: ${ANIENGINE_ENABLE_VULKAN}")
message(STATUS "  HipBLAS: ${ANIENGINE_ENABLE_HIPBLAS}")
message(STATUS "  Metal: ${ANIENGINE_ENABLE_METAL}")
message(STATUS "  OpenCL: ${ANIENGINE_ENABLE_OPENCL}")
message(STATUS "  SYCL: ${ANIENGINE_ENABLE_SYCL}")
message(STATUS "  MUSA: ${ANIENGINE_ENABLE_MUSA}")
if(ANIENGINE_ENABLE_FAST_SOFTMAX)
    message(STATUS "  Fast Softmax: ENABLED")
endif()