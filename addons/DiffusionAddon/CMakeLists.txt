cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)

project(DiffusionAddon)

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

# ============================================
# STABLE DIFFUSION BACKEND OPTIONS
# ============================================
option(SD_CUDA "Enable CUDA backend" OFF)
option(SD_VULKAN "Enable Vulkan backend" OFF)
option(SD_HIPBLAS "Enable HipBLAS backend" OFF)
option(SD_METAL "Enable Metal backend" OFF)
option(SD_OPENCL "Enable OpenCL backend" OFF)
option(SD_SYCL "Enable SYCL backend" OFF)
option(SD_MUSA "Enable MUSA backend" OFF)
option(SD_FAST_SOFTMAX "Enable fast softmax" OFF)

# Backend setup and validation - BEFORE adding stable-diffusion subdirectory
if(SD_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES "89" CACHE STRING "CUDA architectures" FORCE)
    add_definitions(-DSD_USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    message(STATUS "DiffusionAddon: CUDA backend enabled with architecture ${CMAKE_CUDA_ARCHITECTURES}")
    
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDA toolkit not found!")
    endif()
    
    if(SD_FAST_SOFTMAX)
        message(STATUS "DiffusionAddon: Fast softmax enabled")
    endif()
endif()

if(SD_VULKAN)
    add_definitions(-DSD_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    message(STATUS "DiffusionAddon: Vulkan backend enabled")
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan SDK not found!")
    endif()
endif()

if(SD_OPENCL)
    add_definitions(-DSD_USE_OPENCL)
    find_package(OpenCL REQUIRED)
    message(STATUS "DiffusionAddon: OpenCL backend enabled")
    if(NOT OpenCL_FOUND)
        message(FATAL_ERROR "OpenCL not found!")
    endif()
endif()

if(SD_METAL)
    message(STATUS "DiffusionAddon: Metal backend enabled")
    add_definitions(-DSD_USE_METAL)
endif()

if(SD_HIPBLAS)
    message(STATUS "DiffusionAddon: HipBLAS backend enabled")
    add_definitions(-DSD_USE_CUDA)
endif()

if(SD_MUSA)
    message(STATUS "DiffusionAddon: MUSA backend enabled")
    add_definitions(-DSD_USE_CUDA)
endif()

if(SD_SYCL)
    message(STATUS "DiffusionAddon: SYCL backend enabled")
    add_definitions(-DSD_USE_SYCL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing -fsycl")
    if(WIN32)
        set(SYCL_COMPILE_OPTIONS /fp:precise)
    else()
        set(SYCL_COMPILE_OPTIONS -fp-model=precise)
    endif()
endif()

# ============================================
# Get absolute paths
get_filename_component(ANISTUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(CONAN_TOOLCHAIN_PATH "${ANISTUDIO_ROOT}/build/conan/conan_toolchain.cmake" ABSOLUTE)

# Include Conan toolchain
include(${CONAN_TOOLCHAIN_PATH} OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(FATAL_ERROR "Conan toolchain not found at ${CONAN_TOOLCHAIN_PATH}")
endif()

# Find packages
find_package(OpenCV REQUIRED)
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(pybind11 REQUIRED)
find_package(exiv2 CONFIG REQUIRED)

# Try CURL
find_package(CURL QUIET)
if(CURL_FOUND)
    message(STATUS "CURL found")
endif()

# Set external directory
set(EXTERNAL_DIR "${ANISTUDIO_ROOT}/external")

# Add ImGui
if(NOT TARGET ImGui AND EXISTS "${EXTERNAL_DIR}/imgui/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui" "${CMAKE_BINARY_DIR}/imgui")
elseif(TARGET ImGui)
    message(STATUS "ImGui already available")
else()
    message(FATAL_ERROR "ImGui not found at ${EXTERNAL_DIR}/imgui")
endif()

# Add ImGuiFileDialog
if(NOT TARGET ImGuiFileDialog AND EXISTS "${EXTERNAL_DIR}/imgui_filedialog/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui_filedialog" "${CMAKE_BINARY_DIR}/imgui_filedialog")
    message(STATUS "Added ImGuiFileDialog: ${EXTERNAL_DIR}/imgui_filedialog")
elseif(TARGET ImGuiFileDialog)
    message(STATUS "ImGuiFileDialog already available")
else()
    message(WARNING "ImGuiFileDialog not found at ${EXTERNAL_DIR}/imgui_filedialog")
endif()

# Add Zep text editor
if(NOT TARGET Zep::Zep AND EXISTS "${EXTERNAL_DIR}/zep/CMakeLists.txt")
    set(BUILD_IMGUI OFF CACHE BOOL "Build ImGui Demo" FORCE)
    set(BUILD_QT OFF CACHE BOOL "Build Qt Demo" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    set(ZEP_FEATURE_CPP_FILE_SYSTEM ON CACHE BOOL "Enable cpp file system" FORCE)

    add_subdirectory("${EXTERNAL_DIR}/zep" "${CMAKE_BINARY_DIR}/zep")
    message(STATUS "Added Zep: ${EXTERNAL_DIR}/zep")
    set(ZEP_AVAILABLE TRUE)
elseif(TARGET Zep::Zep)
    message(STATUS "Zep already available")
    set(ZEP_AVAILABLE TRUE)
else()
    set(ZEP_AVAILABLE FALSE)
    message(WARNING "Zep not found at ${EXTERNAL_DIR}/zep - text editor features will be limited")
endif()

# Find main libraries
set(MAIN_LIB_DIR "${ANISTUDIO_ROOT}/build/lib")

if(NOT TARGET AniEngineCore)
    find_library(AniEngineCore_LIB NAMES AniEngineCore PATHS "${MAIN_LIB_DIR}" NO_DEFAULT_PATH)
    if(NOT AniEngineCore_LIB)
        message(FATAL_ERROR "AniEngineCore not found")
    endif()
    add_library(AniEngineCore SHARED IMPORTED)
    set_target_properties(AniEngineCore PROPERTIES
        IMPORTED_LOCATION "${AniEngineCore_LIB}"
        IMPORTED_IMPLIB "${AniEngineCore_LIB}")
endif()

if(NOT TARGET AniStudioCore)
    find_library(AniStudioCore_LIB NAMES AniStudioCore PATHS "${MAIN_LIB_DIR}" NO_DEFAULT_PATH)
    if(NOT AniStudioCore_LIB)
        message(FATAL_ERROR "AniStudioCore not found")
    endif()
    add_library(AniStudioCore SHARED IMPORTED)
    set_target_properties(AniStudioCore PROPERTIES
        IMPORTED_LOCATION "${AniStudioCore_LIB}"
        IMPORTED_IMPLIB "${AniStudioCore_LIB}")
endif()

# Find AniStudio executable to link against (for missing symbols)
set(ANISTUDIO_EXE "${ANISTUDIO_ROOT}/build/bin/AniStudio.exe")
if(EXISTS "${ANISTUDIO_EXE}")
    message(STATUS "Found AniStudio executable: ${ANISTUDIO_EXE}")
else()
    message(WARNING "AniStudio executable not found at ${ANISTUDIO_EXE}")
endif()

# Add stable-diffusion.cpp with backend options passed through
# Force stable-diffusion to build as static library
set(SD_BUILD_SHARED_LIBS OFF CACHE BOOL "Build stable-diffusion as static library" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

# Add the subdirectory - stable-diffusion will be built as static lib
add_subdirectory("${EXTERNAL_DIR}/stable-diffusion.cpp" "${CMAKE_BINARY_DIR}/stable-diffusion.cpp")

# Check for stable-diffusion target
if(NOT TARGET stable-diffusion)
    message(WARNING "stable-diffusion target not found - attempting to find library")
    find_library(STABLE_DIFFUSION_LIB NAMES stable-diffusion PATHS "${MAIN_LIB_DIR}" NO_DEFAULT_PATH)
    if(STABLE_DIFFUSION_LIB)
        add_library(stable-diffusion SHARED IMPORTED)
        set_target_properties(stable-diffusion PROPERTIES
            IMPORTED_LOCATION "${STABLE_DIFFUSION_LIB}"
            IMPORTED_IMPLIB "${STABLE_DIFFUSION_LIB}")
        message(STATUS "Found stable-diffusion library: ${STABLE_DIFFUSION_LIB}")
    else()
        message(WARNING "stable-diffusion library not found - some features may not work")
    endif()
endif()

# Collect sources
file(GLOB_RECURSE DIFFUSION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
)
list(APPEND DIFFUSION_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/DiffusionAddon.cpp")

add_library(DiffusionAddon SHARED ${DIFFUSION_SOURCES})

# Include directories
target_include_directories(DiffusionAddon PRIVATE
    ${ANISTUDIO_ROOT}/src/aniengine/ecs
    ${ANISTUDIO_ROOT}/src/aniengine/ecs/base
    ${ANISTUDIO_ROOT}/src/aniengine/ecs/components
    ${ANISTUDIO_ROOT}/src/aniengine/ecs/systems
    ${ANISTUDIO_ROOT}/src/aniengine/plugins
    ${ANISTUDIO_ROOT}/src/aniengine/utils
    ${ANISTUDIO_ROOT}/src/aniengine/utils/media
    ${ANISTUDIO_ROOT}/src/anistudio/gui/base
    ${ANISTUDIO_ROOT}/src/anistudio/gui_utils
    ${ANISTUDIO_ROOT}/src/anistudio/gui_utils/widgets
    ${ANISTUDIO_ROOT}/src/anistudio/gui_utils/ui_schema
    ${ANISTUDIO_ROOT}/src/anistudio/plugins
    ${ANISTUDIO_ROOT}/src/anistudio
    ${ANISTUDIO_ROOT}/src/anistudio/events
    ${ANISTUDIO_ROOT}/src/aniengine/asset_mgr
    ${ANISTUDIO_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ecs
    ecs/components
    ecs/systems
    utils
    gui
)

# External includes
target_include_directories(DiffusionAddon PUBLIC
    "${EXTERNAL_DIR}/glm"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/cr/cr"
    "${EXTERNAL_DIR}/stable-diffusion.cpp"
    "${EXTERNAL_DIR}/stb_image"
    "${EXTERNAL_DIR}/imgui_filedialog"
    "${EXTERNAL_DIR}/imgui_filedialog/stb"
    "${EXTERNAL_DIR}/imgui_nodes"
    "${EXTERNAL_DIR}/ImNodeFlow/include"
    "${EXTERNAL_DIR}/ImGuizmo"
    "${EXTERNAL_DIR}/imgui_markdown"
    "${EXTERNAL_DIR}/zep/include"
)

# Link libraries
target_link_libraries(DiffusionAddon PRIVATE
    AniStudioCore
    AniEngineCore
    ${OpenCV_LIBS}
    GLEW::GLEW
    ZLIB::ZLIB
    PNG::PNG
    glfw
    OpenGL::GL
    exiv2::exiv2
    ImGui
    ImGuiFileDialog
    stable-diffusion
    pybind11::embed
    $<$<PLATFORM_ID:Windows>:kernel32>
    $<$<PLATFORM_ID:Windows>:user32>
)

# Link backend-specific libraries and targets
if(SD_CUDA)
    target_compile_definitions(DiffusionAddon PRIVATE SD_USE_CUDA)
    target_link_libraries(DiffusionAddon PRIVATE 
        CUDA::cudart
        CUDA::cublas
        CUDA::cublasLt
        CUDA::curand
        CUDA::cufft
    )
    
    # Link CUDA backend targets from ggml
    foreach(CUDA_TARGET ggml-cuda ggml_cuda ggml-backend-cuda common-cuda)
        if(TARGET ${CUDA_TARGET})
            target_link_libraries(DiffusionAddon PRIVATE ${CUDA_TARGET})
            message(STATUS "DiffusionAddon: Linked CUDA backend ${CUDA_TARGET}")
        endif()
    endforeach()
endif()

if(SD_VULKAN)
    target_compile_definitions(DiffusionAddon PRIVATE SD_USE_VULKAN)
    target_link_libraries(DiffusionAddon PRIVATE Vulkan::Vulkan)
    
    # Link Vulkan backend targets
    foreach(VULKAN_TARGET ggml-vulkan ggml_vulkan ggml-backend-vulkan common-vulkan)
        if(TARGET ${VULKAN_TARGET})
            target_link_libraries(DiffusionAddon PRIVATE ${VULKAN_TARGET})
            message(STATUS "DiffusionAddon: Linked Vulkan backend ${VULKAN_TARGET}")
        endif()
    endforeach()
endif()

if(SD_OPENCL)
    target_compile_definitions(DiffusionAddon PRIVATE SD_USE_OPENCL)
    target_link_libraries(DiffusionAddon PRIVATE OpenCL::OpenCL)
    
    # Link OpenCL backend targets
    foreach(OPENCL_TARGET ggml-opencl ggml_opencl ggml-backend-opencl common-opencl)
        if(TARGET ${OPENCL_TARGET})
            target_link_libraries(DiffusionAddon PRIVATE ${OPENCL_TARGET})
            message(STATUS "DiffusionAddon: Linked OpenCL backend ${OPENCL_TARGET}")
        endif()
    endforeach()
endif()

if(SD_METAL)
    target_compile_definitions(DiffusionAddon PRIVATE SD_USE_METAL)
    
    # Link Metal backend targets
    foreach(METAL_TARGET ggml-metal ggml_metal ggml-backend-metal common-metal)
        if(TARGET ${METAL_TARGET})
            target_link_libraries(DiffusionAddon PRIVATE ${METAL_TARGET})
            message(STATUS "DiffusionAddon: Linked Metal backend ${METAL_TARGET}")
        endif()
    endforeach()
endif()

if(SD_HIPBLAS)
    target_compile_definitions(DiffusionAddon PRIVATE SD_USE_CUDA)
    
    # Link HipBLAS backend targets
    foreach(HIPBLAS_TARGET ggml-hipblas ggml_hipblas ggml-backend-hipblas common-hipblas)
        if(TARGET ${HIPBLAS_TARGET})
            target_link_libraries(DiffusionAddon PRIVATE ${HIPBLAS_TARGET})
            message(STATUS "DiffusionAddon: Linked HipBLAS backend ${HIPBLAS_TARGET}")
        endif()
    endforeach()
endif()

if(SD_SYCL)
    target_compile_definitions(DiffusionAddon PRIVATE SD_USE_SYCL)
    
    # Link SYCL backend targets
    foreach(SYCL_TARGET ggml-sycl ggml_sycl ggml-backend-sycl common-sycl)
        if(TARGET ${SYCL_TARGET})
            target_link_libraries(DiffusionAddon PRIVATE ${SYCL_TARGET})
            message(STATUS "DiffusionAddon: Linked SYCL backend ${SYCL_TARGET}")
        endif()
    endforeach()
endif()

if(SD_MUSA)
    target_compile_definitions(DiffusionAddon PRIVATE SD_USE_CUDA)
    
    # Link MUSA backend targets
    foreach(MUSA_TARGET ggml-musa ggml_musa ggml-backend-musa common-musa)
        if(TARGET ${MUSA_TARGET})
            target_link_libraries(DiffusionAddon PRIVATE ${MUSA_TARGET})
            message(STATUS "DiffusionAddon: Linked MUSA backend ${MUSA_TARGET}")
        endif()
    endforeach()
endif()

# Link Zep if available
if(ZEP_AVAILABLE AND TARGET Zep::Zep)
    target_link_libraries(DiffusionAddon PRIVATE Zep::Zep)
    target_compile_definitions(DiffusionAddon PRIVATE ZEP_AVAILABLE)
    message(STATUS "Linking Zep text editor to DiffusionAddon")
endif()

if(CURL_FOUND)
    target_link_libraries(DiffusionAddon PRIVATE CURL::libcurl)
    target_compile_definitions(DiffusionAddon PRIVATE HAS_CURL)
endif()

if(WIN32)
    set_target_properties(DiffusionAddon PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS OFF)
    
    # Create def file with plugin exports only
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/DiffusionAddon.def")
    file(WRITE ${DEF_FILE} "EXPORTS\nCreatePlugin\nDestroyPlugin\n")
    set_target_properties(DiffusionAddon PROPERTIES LINK_FLAGS "/DEF:${DEF_FILE}")
    
    # Allow unresolved externals for symbols that should come from the host application
    set_target_properties(DiffusionAddon PROPERTIES 
        LINK_FLAGS "${LINK_FLAGS} /FORCE:UNRESOLVED"
    )
    
    message(STATUS "Note: Some symbols (WindowCloseCallback, Core::Quit, appCore) will be resolved at runtime by the host application")
endif()

target_compile_definitions(DiffusionAddon PRIVATE 
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    PCH_DISABLED
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Apply SYCL compile options if enabled
if(SD_SYCL AND DEFINED SYCL_COMPILE_OPTIONS)
    target_compile_options(DiffusionAddon PRIVATE ${SYCL_COMPILE_OPTIONS})
endif()

# Output directories
set(ADDON_BASE_DIR "${ANISTUDIO_ROOT}/build/addons/DiffusionAddon")
set(ADDON_STAGING_DIR "${ADDON_BASE_DIR}/staging")

file(MAKE_DIRECTORY "${ADDON_BASE_DIR}")
file(MAKE_DIRECTORY "${ADDON_STAGING_DIR}")

set_target_properties(DiffusionAddon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ADDON_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${ADDON_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${ADDON_STAGING_DIR}"
)

message(STATUS "===========================================")
message(STATUS "DiffusionAddon Configuration Summary")
message(STATUS "===========================================")
message(STATUS "Output directory: ${ADDON_STAGING_DIR}")
message(STATUS "Stable-diffusion.h: ${EXTERNAL_DIR}/stable-diffusion.cpp")
message(STATUS "AniEngineCore: ${AniEngineCore_LIB}")
message(STATUS "AniStudioCore: ${AniStudioCore_LIB}")
message(STATUS "Zep available: ${ZEP_AVAILABLE}")
message(STATUS "CURL available: ${CURL_FOUND}")
message(STATUS "")
message(STATUS "Backend Configuration:")
message(STATUS "  CUDA: ${SD_CUDA}")
message(STATUS "  Vulkan: ${SD_VULKAN}")
message(STATUS "  HipBLAS: ${SD_HIPBLAS}")
message(STATUS "  Metal: ${SD_METAL}")
message(STATUS "  OpenCL: ${SD_OPENCL}")
message(STATUS "  SYCL: ${SD_SYCL}")
message(STATUS "  MUSA: ${SD_MUSA}")
if(SD_FAST_SOFTMAX)
    message(STATUS "  Fast Softmax: ENABLED")
endif()
message(STATUS "===========================================")