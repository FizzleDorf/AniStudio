# CMakeLists.txt for AniEngine - Fixed CUDA Backend Linking
cmake_minimum_required(VERSION 3.12)
project(AniEngine VERSION 1.0.0 LANGUAGES CXX)

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

# Set stable-diffusion.cpp backend options based on AniEngine options
set(SD_CUDA ${ANIENGINE_ENABLE_CUDA})
set(SD_VULKAN ${ANIENGINE_ENABLE_VULKAN})
set(SD_HIPBLAS ${ANIENGINE_ENABLE_HIPBLAS})
set(SD_METAL ${ANIENGINE_ENABLE_METAL})
set(SD_OPENCL ${ANIENGINE_ENABLE_OPENCL})
set(SD_SYCL ${ANIENGINE_ENABLE_SYCL})
set(SD_MUSA ${ANIENGINE_ENABLE_MUSA})
set(SD_FAST_SOFTMAX ${ANIENGINE_ENABLE_FAST_SOFTMAX})

# Backend setup and validation
if(ANIENGINE_ENABLE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CUDA_ARCHITECTURES "89" CACHE STRING "CUDA architectures")
    add_definitions(-DSD_USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "AniEngine: CUDA backend enabled")
    
    # Verify CUDA toolkit components
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDA toolkit not found!")
    endif()
    
    # Check for required CUDA libraries
    if(NOT TARGET CUDA::cudart)
        message(FATAL_ERROR "CUDA runtime library not found!")
    endif()
    
    if(NOT TARGET CUDA::cublas)
        message(FATAL_ERROR "CUDA BLAS library not found!")
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

if(ANIENGINE_ENABLE_HIPBLAS)
    add_definitions(-DSD_USE_CUDA)  # HipBLAS uses same define as CUDA
    message(STATUS "AniEngine: HipBLAS backend enabled")
endif()

if(ANIENGINE_ENABLE_METAL)
    add_definitions(-DSD_USE_METAL)
    message(STATUS "AniEngine: Metal backend enabled")
    if(NOT APPLE)
        message(WARNING "Metal backend is only supported on Apple platforms")
    endif()
endif()

if(ANIENGINE_ENABLE_OPENCL)
    add_definitions(-DSD_USE_OPENCL)
    find_package(OpenCL QUIET)
    if(NOT OpenCL_FOUND)
        message(WARNING "OpenCL not found - make sure OpenCL headers and ICD loader are installed")
    endif()
    message(STATUS "AniEngine: OpenCL backend enabled")
endif()

if(ANIENGINE_ENABLE_SYCL)
    add_definitions(-DSD_USE_SYCL)
    message(STATUS "AniEngine: SYCL backend enabled")
    message(STATUS "Run 'source /opt/intel/oneapi/setvars.sh' before building (Linux)")
    message(STATUS "Run oneAPI setvars.bat before building (Windows)")
endif()

if(ANIENGINE_ENABLE_MUSA)
    add_definitions(-DSD_USE_CUDA)  # MUSA uses same define as CUDA
    message(STATUS "AniEngine: MUSA backend enabled")
endif()

# Find dependencies if building standalone
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - find all dependencies
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
    
    # Add external dependencies when building standalone
    if(EXISTS "${CMAKE_SOURCE_DIR}/../../external/stable-diffusion.cpp")
        # Pass backend options to stable-diffusion.cpp before adding subdirectory
        set(SD_CUDA ${SD_CUDA} CACHE BOOL "CUDA backend for stable-diffusion" FORCE)
        set(SD_VULKAN ${SD_VULKAN} CACHE BOOL "Vulkan backend for stable-diffusion" FORCE)
        set(SD_HIPBLAS ${SD_HIPBLAS} CACHE BOOL "HipBLAS backend for stable-diffusion" FORCE)
        set(SD_METAL ${SD_METAL} CACHE BOOL "Metal backend for stable-diffusion" FORCE)
        set(SD_OPENCL ${SD_OPENCL} CACHE BOOL "OpenCL backend for stable-diffusion" FORCE)
        set(SD_SYCL ${SD_SYCL} CACHE BOOL "SYCL backend for stable-diffusion" FORCE)
        set(SD_MUSA ${SD_MUSA} CACHE BOOL "MUSA backend for stable-diffusion" FORCE)
        set(SD_FAST_SOFTMAX ${SD_FAST_SOFTMAX} CACHE BOOL "Fast softmax for stable-diffusion" FORCE)
        
        add_subdirectory(${CMAKE_SOURCE_DIR}/../../external/stable-diffusion.cpp ${CMAKE_BINARY_DIR}/stable-diffusion)
    else()
        message(FATAL_ERROR "stable-diffusion.cpp not found. Required for AniEngine.")
    endif()
else()
    # Parent project build - add external dependencies here to avoid rebuilding
    if(NOT TARGET stable-diffusion)
        # Pass backend options to stable-diffusion.cpp before adding subdirectory
        set(SD_CUDA ${SD_CUDA} CACHE BOOL "CUDA backend for stable-diffusion" FORCE)
        set(SD_VULKAN ${SD_VULKAN} CACHE BOOL "Vulkan backend for stable-diffusion" FORCE)
        set(SD_HIPBLAS ${SD_HIPBLAS} CACHE BOOL "HipBLAS backend for stable-diffusion" FORCE)
        set(SD_METAL ${SD_METAL} CACHE BOOL "Metal backend for stable-diffusion" FORCE)
        set(SD_OPENCL ${SD_OPENCL} CACHE BOOL "OpenCL backend for stable-diffusion" FORCE)
        set(SD_SYCL ${SD_SYCL} CACHE BOOL "SYCL backend for stable-diffusion" FORCE)
        set(SD_MUSA ${SD_MUSA} CACHE BOOL "MUSA backend for stable-diffusion" FORCE)
        set(SD_FAST_SOFTMAX ${SD_FAST_SOFTMAX} CACHE BOOL "Fast softmax for stable-diffusion" FORCE)
        
        add_subdirectory(${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp ${CMAKE_BINARY_DIR}/external/stable-diffusion)
    endif()
endif()

# Collect source files including plugin system
file(GLOB_RECURSE ANIENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp"  # Include integrated plugin system
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

# Include directories
target_include_directories(AniEngineCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    #ecs base
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/base>
    # systems
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems>
    # components
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/SDCPPComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/ImageComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/VideoComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/ScriptComponents>
    # plugins (integrated)
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins>
    # utils
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/defaults>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/media>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/scripts>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/sdcpp>
    # interface
    $<INSTALL_INTERFACE:include/aniengine>
)

# Add root include directory for public headers
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - include directory is at ../../include
    target_include_directories(AniEngineCore PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../../include>
    )
else()
    # Parent project build - include directory is at root/include
    target_include_directories(AniEngineCore PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    )
endif()

# External includes (when building standalone)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    target_include_directories(AniEngineCore PUBLIC
        ${CMAKE_SOURCE_DIR}/../../external/glm
        ${CMAKE_SOURCE_DIR}/../../external/stable-diffusion.cpp
        ${CMAKE_SOURCE_DIR}/../../external/nlohmann_json
        ${CMAKE_SOURCE_DIR}/../../external/cr/cr
    )
else()
    # Parent project includes
    target_include_directories(AniEngineCore PUBLIC
        ${CMAKE_SOURCE_DIR}/external/glm
        ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
        ${CMAKE_SOURCE_DIR}/external/nlohmann_json
        ${CMAKE_SOURCE_DIR}/external/cr/cr
    )
endif()

# Link libraries - FIXED CUDA LINKING
target_link_libraries(AniEngineCore PUBLIC
    # External dependencies
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

# CUDA support - CRITICAL FIX: Link all required CUDA libraries
if(ANIENGINE_ENABLE_CUDA)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)
    target_link_libraries(AniEngineCore PUBLIC 
        CUDA::cudart
        CUDA::cublas
        CUDA::cublasLt
        CUDA::curand
        CUDA::cufft
    )
    
    # For stable-diffusion.cpp CUDA backend, also need these libraries
    # Check if ggml-cuda target exists (from stable-diffusion.cpp)
    if(TARGET ggml-cuda)
        target_link_libraries(AniEngineCore PUBLIC ggml-cuda)
        message(STATUS "Linked ggml-cuda backend")
    else()
        message(WARNING "ggml-cuda target not found - CUDA backend may not link properly")
    endif()
    
    # Alternative: Link the backend library directly if it exists
    if(TARGET ggml-backend-cuda)
        target_link_libraries(AniEngineCore PUBLIC ggml-backend-cuda)
        message(STATUS "Linked ggml-backend-cuda")
    endif()
endif()

# Vulkan support
if(ANIENGINE_ENABLE_VULKAN)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_VULKAN)
    
    # Link Vulkan backend if available
    if(TARGET ggml-vulkan)
        target_link_libraries(AniEngineCore PUBLIC ggml-vulkan)
        message(STATUS "Linked ggml-vulkan backend")
    endif()
    
    if(TARGET ggml-backend-vulkan)
        target_link_libraries(AniEngineCore PUBLIC ggml-backend-vulkan)
        message(STATUS "Linked ggml-backend-vulkan")
    endif()
endif()

# OpenCL support
if(ANIENGINE_ENABLE_OPENCL AND OpenCL_FOUND)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_OPENCL)
    target_link_libraries(AniEngineCore PUBLIC OpenCL::OpenCL)
    
    # Link OpenCL backend if available
    if(TARGET ggml-opencl)
        target_link_libraries(AniEngineCore PUBLIC ggml-opencl)
        message(STATUS "Linked ggml-opencl backend")
    endif()
endif()

# Metal support
if(ANIENGINE_ENABLE_METAL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_METAL)
    
    # Link Metal backend if available
    if(TARGET ggml-metal)
        target_link_libraries(AniEngineCore PUBLIC ggml-metal)
        message(STATUS "Linked ggml-metal backend")
    endif()
endif()

# HipBLAS support
if(ANIENGINE_ENABLE_HIPBLAS)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)  # HipBLAS uses CUDA define
    
    # Link HipBLAS backend if available
    if(TARGET ggml-hipblas)
        target_link_libraries(AniEngineCore PUBLIC ggml-hipblas)
        message(STATUS "Linked ggml-hipblas backend")
    endif()
endif()

# SYCL support
if(ANIENGINE_ENABLE_SYCL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_SYCL)
    
    # Link SYCL backend if available
    if(TARGET ggml-sycl)
        target_link_libraries(AniEngineCore PUBLIC ggml-sycl)
        message(STATUS "Linked ggml-sycl backend")
    endif()
endif()

# MUSA support
if(ANIENGINE_ENABLE_MUSA)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)  # MUSA uses CUDA define
    
    # Link MUSA backend if available
    if(TARGET ggml-musa)
        target_link_libraries(AniEngineCore PUBLIC ggml-musa)
        message(STATUS "Linked ggml-musa backend")
    endif()
endif()

# Compile definitions
target_compile_definitions(AniEngineCore PUBLIC
    GLEW_STATIC
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Backend-specific compile definitions
if(ANIENGINE_ENABLE_VULKAN)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_VULKAN)
endif()

if(ANIENGINE_ENABLE_HIPBLAS)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)  # HipBLAS uses CUDA define
endif()

if(ANIENGINE_ENABLE_METAL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_METAL)
endif()

if(ANIENGINE_ENABLE_OPENCL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_OPENCL)
endif()

if(ANIENGINE_ENABLE_SYCL)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_SYCL)
endif()

if(ANIENGINE_ENABLE_MUSA)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)  # MUSA uses CUDA define
endif()

# Create an alias for consistent naming
add_library(AniEngine::AniEngineCore ALIAS AniEngineCore)

message(STATUS "AniEngine configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library type: ${BUILD_SHARED_LIBS}")
message(STATUS "  Plugin Support: INTEGRATED (engine/plugins)")
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

# Debug: Print all targets created by stable-diffusion.cpp
get_directory_property(TARGETS BUILDSYSTEM_TARGETS)
message(STATUS "Available targets from stable-diffusion.cpp:")
foreach(TARGET ${TARGETS})
    if(TARGET ${TARGET})
        get_target_property(TARGET_TYPE ${TARGET} TYPE)
        if(TARGET_TYPE MATCHES "STATIC_LIBRARY|SHARED_LIBRARY|EXECUTABLE")
            message(STATUS "  - ${TARGET} (${TARGET_TYPE})")
        endif()
    endif()
endforeach()