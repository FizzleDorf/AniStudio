cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)

project(AniStudio-SDCPP)

if(UNIX AND NOT APPLE)
    set(ENV{CPATH} "")
    set(ENV{CPLUS_INCLUDE_PATH} "")
    set(ENV{C_INCLUDE_PATH} "")
    set(ENV{INCLUDE} "")
    set(ENV{MSYSTEM} "")
    set(ENV{MSYSCON} "")
    set(ENV{MINGW_PREFIX} "")
    
    if(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
        list(FILTER CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES EXCLUDE REGEX "/mnt/.*")
        list(FILTER CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES EXCLUDE REGEX "/c/.*")
        list(FILTER CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES EXCLUDE REGEX "msys64")
        list(FILTER CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES EXCLUDE REGEX "ucrt64")
        list(FILTER CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES EXCLUDE REGEX "mingw")
    endif()
endif()

if(WIN32)
    set(CMAKE_JOB_POOLS compile=1 link=1)
    set(CMAKE_JOB_POOL_COMPILE compile)
    set(CMAKE_JOB_POOL_LINK link)
    set(CMAKE_POLICY_DEFAULT_CMP0114 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-D_GNU_SOURCE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

option(SD_CUDA "Enable CUDA backend" OFF)
option(SD_VULKAN "Enable Vulkan backend" OFF)
option(SD_HIPBLAS "Enable HipBLAS backend" OFF)
option(SD_METAL "Enable Metal backend" OFF)
option(SD_OPENCL "Enable OpenCL backend" OFF)
option(SD_SYCL "Enable SYCL backend" OFF)
option(SD_MUSA "Enable MUSA backend" OFF)
option(SD_FAST_SOFTMAX "Enable fast softmax" OFF)

option(SD_USE_SYSTEM_GGML "Use system-installed GGML library" OFF)

if(SD_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES "89" CACHE STRING "CUDA architectures" FORCE)
    add_definitions(-DSD_USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDA toolkit not found!")
    endif()
endif()

if(SD_VULKAN)
    add_definitions(-DSD_USE_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

if(SD_OPENCL)
    add_definitions(-DSD_USE_OPENCL)
    find_package(OpenCL REQUIRED)
endif()

if(SD_METAL)
    add_definitions(-DSD_USE_METAL)
endif()

if(SD_HIPBLAS OR SD_MUSA)
    add_definitions(-DSD_USE_CUDA)
endif()

if(SD_SYCL)
    add_definitions(-DSD_USE_SYCL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing -fsycl")
    if(WIN32)
        set(SYCL_COMPILE_OPTIONS /fp:precise)
    else()
        set(SYCL_COMPILE_OPTIONS -fp-model=precise)
    endif()
endif()

set(BACKEND_SUFFIX "")
if(SD_CUDA)
    set(BACKEND_SUFFIX "_CUDA")
elseif(SD_VULKAN)
    set(BACKEND_SUFFIX "_VULKAN")
elseif(SD_HIPBLAS)
    set(BACKEND_SUFFIX "_HIP")
elseif(SD_METAL)
    set(BACKEND_SUFFIX "_METAL")
elseif(SD_OPENCL)
    set(BACKEND_SUFFIX "_OPENCL")
elseif(SD_SYCL)
    set(BACKEND_SUFFIX "_SYCL")
elseif(SD_MUSA)
    set(BACKEND_SUFFIX "_MUSA")
else()
    set(BACKEND_SUFFIX "_CPU")
endif()

get_filename_component(ANISTUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(CONAN_TOOLCHAIN_PATH "${ANISTUDIO_ROOT}/build/conan/conan_toolchain.cmake" ABSOLUTE)

include(${CONAN_TOOLCHAIN_PATH} OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(FATAL_ERROR "Conan toolchain not found at ${CONAN_TOOLCHAIN_PATH}")
endif()

find_package(OpenCV REQUIRED)
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(pybind11 REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(CURL QUIET)

if(SD_USE_SYSTEM_GGML)
    if(NOT ggml_DIR AND DEFINED CONAN_LLAMA_CPP_ROOT)
        set(ggml_DIR "${CONAN_LLAMA_CPP_ROOT}/lib/cmake/ggml")
    endif()
    find_package(ggml CONFIG REQUIRED)
endif()

set(EXTERNAL_DIR "${ANISTUDIO_ROOT}/external")

if(NOT TARGET ImGui AND EXISTS "${EXTERNAL_DIR}/imgui/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui" "${CMAKE_BINARY_DIR}/imgui")
endif()

set(NFD_AVAILABLE FALSE)
if(EXISTS "${EXTERNAL_DIR}/nativefiledialog-extended/CMakeLists.txt" AND NOT TARGET nfd)
    add_subdirectory("${EXTERNAL_DIR}/nativefiledialog-extended" "${CMAKE_BINARY_DIR}/_external/nativefiledialog-extended")
    set(NFD_AVAILABLE TRUE)
elseif(TARGET nfd)
    set(NFD_AVAILABLE TRUE)
endif()

if(NOT TARGET Zep::Zep AND EXISTS "${EXTERNAL_DIR}/zep/CMakeLists.txt")
    set(BUILD_IMGUI OFF CACHE BOOL "" FORCE)
    set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(ZEP_FEATURE_CPP_FILE_SYSTEM ON CACHE BOOL "" FORCE)
    add_subdirectory("${EXTERNAL_DIR}/zep" "${CMAKE_BINARY_DIR}/zep")
    set(ZEP_AVAILABLE TRUE)
elseif(TARGET Zep::Zep)
    set(ZEP_AVAILABLE TRUE)
else()
    set(ZEP_AVAILABLE FALSE)
endif()

set(MAIN_LIB_DIR "${ANISTUDIO_ROOT}/build/lib")
set(MAIN_BIN_DIR "${ANISTUDIO_ROOT}/build/bin")

if(WIN32)
    if(EXISTS "${MAIN_LIB_DIR}/AniEngineCore.lib")
        add_library(AniEngineCore SHARED IMPORTED)
        set_target_properties(AniEngineCore PROPERTIES
            IMPORTED_IMPLIB "${MAIN_LIB_DIR}/AniEngineCore.lib"
            IMPORTED_LOCATION "${MAIN_BIN_DIR}/AniEngineCore.dll"
        )
        message(STATUS "Found AniEngineCore: ${MAIN_LIB_DIR}/AniEngineCore.lib -> ${MAIN_BIN_DIR}/AniEngineCore.dll")
    else()
        message(FATAL_ERROR "AniEngineCore.lib not found in ${MAIN_LIB_DIR}")
    endif()
    
    if(EXISTS "${MAIN_LIB_DIR}/AniStudioCore.lib")
        add_library(AniStudioCore SHARED IMPORTED)
        set_target_properties(AniStudioCore PROPERTIES
            IMPORTED_IMPLIB "${MAIN_LIB_DIR}/AniStudioCore.lib"
            IMPORTED_LOCATION "${MAIN_BIN_DIR}/AniStudioCore.dll"
        )
        message(STATUS "Found AniStudioCore: ${MAIN_LIB_DIR}/AniStudioCore.lib -> ${MAIN_BIN_DIR}/AniStudioCore.dll")
    else()
        message(FATAL_ERROR "AniStudioCore.lib not found in ${MAIN_LIB_DIR}")
    endif()
else()
    if(EXISTS "${MAIN_LIB_DIR}/AniEngineCore.so")
        add_library(AniEngineCore SHARED IMPORTED)
        set_target_properties(AniEngineCore PROPERTIES 
            IMPORTED_LOCATION "${MAIN_LIB_DIR}/AniEngineCore.so"
        )
        message(STATUS "Found AniEngineCore: ${MAIN_LIB_DIR}/AniEngineCore.so")
    elseif(EXISTS "${MAIN_LIB_DIR}/libAniEngineCore.so")
        add_library(AniEngineCore SHARED IMPORTED)
        set_target_properties(AniEngineCore PROPERTIES 
            IMPORTED_LOCATION "${MAIN_LIB_DIR}/libAniEngineCore.so"
        )
        message(STATUS "Found AniEngineCore: ${MAIN_LIB_DIR}/libAniEngineCore.so")
    else()
        message(FATAL_ERROR "AniEngineCore.so not found in ${MAIN_LIB_DIR}")
    endif()
    
    if(EXISTS "${MAIN_LIB_DIR}/AniStudioCore.so")
        add_library(AniStudioCore SHARED IMPORTED)
        set_target_properties(AniStudioCore PROPERTIES 
            IMPORTED_LOCATION "${MAIN_LIB_DIR}/AniStudioCore.so"
        )
        message(STATUS "Found AniStudioCore: ${MAIN_LIB_DIR}/AniStudioCore.so")
    elseif(EXISTS "${MAIN_LIB_DIR}/libAniStudioCore.so")
        add_library(AniStudioCore SHARED IMPORTED)
        set_target_properties(AniStudioCore PROPERTIES 
            IMPORTED_LOCATION "${MAIN_LIB_DIR}/libAniStudioCore.so"
        )
        message(STATUS "Found AniStudioCore: ${MAIN_LIB_DIR}/libAniStudioCore.so")
    else()
        message(FATAL_ERROR "AniStudioCore.so not found in ${MAIN_LIB_DIR}")
    endif()
endif()

set(SD_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(WIN32)
    set(ORIGINAL_PATH "$ENV{PATH}")
    set(ENV{PATH} "")
    foreach(TOKEN ${ORIGINAL_PATH})
        string(FIND "${TOKEN}" "msys64" FOUND)
        if(FOUND EQUAL -1)
            if(NOT "$ENV{PATH}" STREQUAL "")
                set(ENV{PATH} "$ENV{PATH};${TOKEN}")
            else()
                set(ENV{PATH} "${TOKEN}")
            endif()
        endif()
    endforeach()
    set(ENV{INCLUDE} "")
    set(ENV{CPATH} "")
    set(ENV{C_INCLUDE_PATH} "")
    set(ENV{CPLUS_INCLUDE_PATH} "")
    set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "")
    set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "")
    message(STATUS "Temporarily removed MinGW from PATH for stable-diffusion.cpp")
endif()

if(SD_USE_SYSTEM_GGML)
    set(SD_USE_SYSTEM_GGML ON CACHE BOOL "" FORCE)
endif()

if(EXISTS "${EXTERNAL_DIR}/stable-diffusion.cpp/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/stable-diffusion.cpp" "${CMAKE_BINARY_DIR}/stable-diffusion.cpp")
endif()

if(WIN32)
    set(ENV{PATH} "${ORIGINAL_PATH}")
    message(STATUS "Restored original PATH after stable-diffusion.cpp")
endif()

if(NOT TARGET stable-diffusion)
    find_library(STABLE_DIFFUSION_LIB NAMES libstable-diffusion.a stable-diffusion.a libstable-diffusion.so 
                 PATHS "${MAIN_LIB_DIR}" "${CMAKE_BINARY_DIR}/stable-diffusion.cpp")
    if(STABLE_DIFFUSION_LIB)
        add_library(stable-diffusion UNKNOWN IMPORTED)
        set_target_properties(stable-diffusion PROPERTIES IMPORTED_LOCATION "${STABLE_DIFFUSION_LIB}"
                              INTERFACE_INCLUDE_DIRECTORIES "${EXTERNAL_DIR}/stable-diffusion.cpp")
    endif()
endif()

file(GLOB_RECURSE DIFFUSION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
    "${ANISTUDIO_ROOT}/src/anistudio/gui_utils/states/*.cpp"
)
list(APPEND DIFFUSION_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/AniStudio-SDCPP.cpp")

set(TARGET_NAME "AniStudio-SDCPP${BACKEND_SUFFIX}")
add_library(${TARGET_NAME} SHARED ${DIFFUSION_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    ${ANISTUDIO_ROOT}/src/aniengine
    ${ANISTUDIO_ROOT}/src/aniengine/ecs
    ${ANISTUDIO_ROOT}/src/aniengine/ecs/base
    ${ANISTUDIO_ROOT}/src/aniengine/ecs/components
    ${ANISTUDIO_ROOT}/src/aniengine/ecs/systems
    ${ANISTUDIO_ROOT}/src/aniengine/plugins
    ${ANISTUDIO_ROOT}/src/aniengine/utils
    ${ANISTUDIO_ROOT}/src/aniengine/utils/media
    ${ANISTUDIO_ROOT}/src/anistudio/gui/base
    ${ANISTUDIO_ROOT}/src/anistudio/utils
    ${ANISTUDIO_ROOT}/src/anistudio/utils/states
    ${ANISTUDIO_ROOT}/src/anistudio/utils/widgets
    ${ANISTUDIO_ROOT}/src/anistudio/utils/ui_schema
    ${ANISTUDIO_ROOT}/src/anistudio/utils/rendering
    ${ANISTUDIO_ROOT}/src/anistudio/plugins
    ${ANISTUDIO_ROOT}/src/anistudio/gui/settings
    ${ANISTUDIO_ROOT}/src/anistudio
    ${ANISTUDIO_ROOT}/src/anistudio/events
    ${ANISTUDIO_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ecs
    ecs/components
    ecs/systems
    utils
    gui
)

target_include_directories(${TARGET_NAME} PUBLIC
    "${EXTERNAL_DIR}/glm"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/cr/cr"
    "${EXTERNAL_DIR}/stable-diffusion.cpp"
    "${EXTERNAL_DIR}/stb_image"
    "${EXTERNAL_DIR}/imgui_nodes"
    "${EXTERNAL_DIR}/ImNodeFlow/include"
    "${EXTERNAL_DIR}/ImGuizmo"
    "${EXTERNAL_DIR}/imgui_markdown"
    "${EXTERNAL_DIR}/zep/include"
    "${EXTERNAL_DIR}/nativefiledialog-extended/include"
)

target_link_libraries(${TARGET_NAME} PRIVATE
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
    stable-diffusion
    pybind11::embed
    nfd
)

if(SD_USE_SYSTEM_GGML)
    target_link_libraries(${TARGET_NAME} PRIVATE ggml::ggml)
endif()

if(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE
        ws2_32
        winmm
        shlwapi
    )
    target_link_options(${TARGET_NAME} PRIVATE
        "/FORCE:UNRESOLVED"
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${TARGET_NAME} PRIVATE
        dl
        pthread
        stdc++fs
        rt
    )
    target_link_options(${TARGET_NAME} PRIVATE
        "-Wl,--unresolved-symbols=ignore-all"
    )
endif()

if(SD_CUDA)
    target_link_libraries(${TARGET_NAME} PRIVATE CUDA::cudart CUDA::cublas CUDA::curand)
    foreach(CUDA_T ggml-cuda ggml_cuda ggml-backend-cuda common-cuda)
        if(TARGET ${CUDA_T})
            target_link_libraries(${TARGET_NAME} PRIVATE ${CUDA_T})
        endif()
    endforeach()
endif()

if(ZEP_AVAILABLE AND TARGET Zep::Zep)
    target_link_libraries(${TARGET_NAME} PRIVATE Zep::Zep)
    target_compile_definitions(${TARGET_NAME} PRIVATE ZEP_AVAILABLE)
endif()

if(CURL_FOUND)
    target_link_libraries(${TARGET_NAME} PRIVATE CURL::libcurl)
    target_compile_definitions(${TARGET_NAME} PRIVATE HAS_CURL)
endif()

if(WIN32)
    set_target_properties(${TARGET_NAME} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        PREFIX ""
    )
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.def")
    file(WRITE ${DEF_FILE} "EXPORTS\nCreatePlugin\nDestroyPlugin\n")
    target_sources(${TARGET_NAME} PRIVATE ${DEF_FILE})
else()
    set_target_properties(${TARGET_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        PREFIX ""
    )
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE 
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    PCH_DISABLED
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    NFD_AVAILABLE
)

set(ADDON_STAGING_DIR "${ANISTUDIO_ROOT}/build/plugins/${TARGET_NAME}/staging")
file(MAKE_DIRECTORY "${ADDON_STAGING_DIR}")

set_target_properties(${TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${ADDON_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${ADDON_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ADDON_STAGING_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${ADDON_STAGING_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${ADDON_STAGING_DIR}"
)

message(STATUS "Configured ${TARGET_NAME} for Staging: ${ADDON_STAGING_DIR}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
if(WIN32)
    message(STATUS "Windows build: Looking for DLLs in ${MAIN_BIN_DIR}")
endif()