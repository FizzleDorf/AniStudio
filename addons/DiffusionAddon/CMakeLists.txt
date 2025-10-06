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

# Get absolute paths
get_filename_component(ANISTUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(CONAN_TOOLCHAIN_PATH "${ANISTUDIO_ROOT}/build/conan/conan_toolchain.cmake" ABSOLUTE)

# Include Conan toolchain
include(${CONAN_TOOLCHAIN_PATH} OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(FATAL_ERROR "Conan toolchain not found at ${CONAN_TOOLCHAIN_PATH}")
endif()

# Find packages
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(FFmpeg REQUIRED)

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

add_subdirectory("${EXTERNAL_DIR}/stable-diffusion.cpp" "${CMAKE_BINARY_DIR}/stable-diffusion.cpp")

# Check for stable-diffusion target (should be built by AniEngine)
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

# Include directories - MATCH ExamplePlugin + stable-diffusion.h path
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
    ${ANISTUDIO_ROOT}/src/anistudio/gui_utils/ui_schema
    ${ANISTUDIO_ROOT}/src/anistudio/plugins
    ${ANISTUDIO_ROOT}/src/anistudio
    ${ANISTUDIO_ROOT}/src/anistudio/events
    ${ANISTUDIO_ROOT}/src/aniengine/utils
    ${ANISTUDIO_ROOT}/src/aniengine/asset_mgr
    ${ANISTUDIO_ROOT}/include
    ${EXTERNAL_DIR}/nlohmann_json
    ${EXTERNAL_DIR}/stable-diffusion.cpp
    ${EXTERNAL_DIR}/imgui
    ${EXTERNAL_DIR}/stb_image
    ${EXTERNAL_DIR}/imgui_filedialog
    ${EXTERNAL_DIR}/imgui_filedialog/stb
    ${EXTERNAL_DIR}/glm
    ${EXTERNAL_DIR}/stable-diffusion.cpp  # This is where stable-diffusion.h lives
    ${CMAKE_CURRENT_SOURCE_DIR}
    ecs
    ecs/components
    ecs/systems
    utils
    gui
)

# Link libraries - MATCH ExamplePlugin + stable-diffusion
target_link_libraries(DiffusionAddon PRIVATE
    AniStudioCore
    AniEngineCore
    GLEW::GLEW
    ZLIB::ZLIB
    PNG::PNG
    glfw
    OpenGL::GL
    ImGui
	stable-diffusion
    $<$<PLATFORM_ID:Windows>:kernel32>
    $<$<PLATFORM_ID:Windows>:user32>
)

# Link stable-diffusion if available
if(TARGET stable-diffusion)
    target_link_libraries(DiffusionAddon PRIVATE stable-diffusion)
    message(STATUS "Linking stable-diffusion target")
endif()

if(CURL_FOUND)
    target_link_libraries(DiffusionAddon PRIVATE CURL::libcurl)
    target_compile_definitions(DiffusionAddon PRIVATE HAS_CURL)
endif()

if(WIN32)
    set_target_properties(DiffusionAddon PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS OFF)
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/DiffusionAddon.def")
    file(WRITE ${DEF_FILE} "EXPORTS\nCreatePlugin\nDestroyPlugin\n")
    set_target_properties(DiffusionAddon PROPERTIES LINK_FLAGS "/DEF:${DEF_FILE}")
endif()

target_compile_definitions(DiffusionAddon PRIVATE 
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    PCH_DISABLED
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

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

message(STATUS "DiffusionAddon output: ${ADDON_STAGING_DIR}")
message(STATUS "DiffusionAddon stable-diffusion.h path: ${EXTERNAL_DIR}/stable-diffusion.cpp")