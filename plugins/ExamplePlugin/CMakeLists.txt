# Plugin CMakeLists.txt - FIXED to create subdirectory
cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)

project(ExamplePlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform specific settings
if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(UNIX AND NOT APPLE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

# Include Conan toolchain from correct location
include(../../build/conan/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(FATAL_ERROR "Conan toolchain not found at ../../build/conan/conan_toolchain.cmake. Please run the main project build first.")
endif()

# Find packages
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# Get absolute path to AniStudio root directory
get_filename_component(ANISTUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(EXTERNAL_DIR "${ANISTUDIO_ROOT}/external")

# Add ImGui using absolute path
if(NOT TARGET ImGui AND EXISTS "${EXTERNAL_DIR}/imgui/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui" "${CMAKE_BINARY_DIR}/imgui")
    message(STATUS "Added ImGui for plugin from: ${EXTERNAL_DIR}/imgui")
elseif(TARGET ImGui)
    message(STATUS "ImGui target already available")
else()
    message(FATAL_ERROR "ImGui CMakeLists.txt not found")
endif()

# Try to find or build the main libraries
if(NOT TARGET AniEngineCore)
    find_library(AniEngineCore_LIB 
        NAMES AniEngineCore 
        PATHS "../../build/lib" 
        NO_DEFAULT_PATH)
    
    if(NOT AniEngineCore_LIB)
        message(FATAL_ERROR "AniEngineCore library not found. Please build the main project first.")
    endif()
    
    add_library(AniEngineCore SHARED IMPORTED)
    set_target_properties(AniEngineCore PROPERTIES
        IMPORTED_LOCATION "${AniEngineCore_LIB}"
        IMPORTED_IMPLIB "${AniEngineCore_LIB}"
    )
endif()

if(NOT TARGET AniStudioCore)
    find_library(AniStudioCore_LIB 
        NAMES AniStudioCore 
        PATHS "../../build/lib" 
        NO_DEFAULT_PATH)
    
    if(NOT AniStudioCore_LIB)
        message(FATAL_ERROR "AniStudioCore library not found. Please build the main project first.")
    endif()
    
    add_library(AniStudioCore SHARED IMPORTED)
    set_target_properties(AniStudioCore PROPERTIES
        IMPORTED_LOCATION "${AniStudioCore_LIB}"
        IMPORTED_IMPLIB "${AniStudioCore_LIB}"
    )
endif()

# Create the plugin DLL
add_library(ExamplePlugin SHARED
    ExamplePlugin.cpp
)

# Include paths
target_include_directories(ExamplePlugin PRIVATE
    ../../src/aniengine/ecs
    ../../src/aniengine/ecs/base
    ../../src/aniengine/plugins
    ../../src/anistudio/gui/base
    ../../src/anistudio/plugins
    ../../src/aniengine/utils
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/imgui"
    "${EXTERNAL_DIR}/glm"
    ../../include
)

# Link libraries
target_link_libraries(ExamplePlugin PRIVATE
    AniStudioCore
    AniEngineCore
    GLEW::GLEW
    glfw
    OpenGL::GL
    ImGui
    $<$<PLATFORM_ID:Windows>:kernel32>
    $<$<PLATFORM_ID:Windows>:user32>
    $<$<PLATFORM_ID:Windows>:gdi32>
    $<$<PLATFORM_ID:Windows>:winspool>
    $<$<PLATFORM_ID:Windows>:shell32>
    $<$<PLATFORM_ID:Windows>:ole32>
    $<$<PLATFORM_ID:Windows>:oleaut32>
    $<$<PLATFORM_ID:Windows>:uuid>
    $<$<PLATFORM_ID:Windows>:comdlg32>
    $<$<PLATFORM_ID:Windows>:advapi32>
)

# FIXED: Create ExamplePlugin subdirectory and put DLL there
get_filename_component(ABS_ANISTUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(PLUGINS_BASE_DIR "${ABS_ANISTUDIO_ROOT}/build/plugins")
set(PLUGIN_TARGET_DIR "${PLUGINS_BASE_DIR}/ExamplePlugin")

# Create the directories
file(MAKE_DIRECTORY "${PLUGINS_BASE_DIR}")
file(MAKE_DIRECTORY "${PLUGIN_TARGET_DIR}")

# Force DLL to go into the ExamplePlugin subdirectory
set_target_properties(ExamplePlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_TARGET_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_TARGET_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PLUGIN_TARGET_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PLUGIN_TARGET_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PLUGIN_TARGET_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PLUGIN_TARGET_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PLUGIN_TARGET_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PLUGIN_TARGET_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PLUGIN_TARGET_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${PLUGIN_TARGET_DIR}"
)

# DLL export settings - FIXED: Only request the two functions your plugin system actually uses
if(WIN32)
    set_target_properties(ExamplePlugin PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
    
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/ExamplePlugin.def")
    file(WRITE ${DEF_FILE} "EXPORTS\n")
    file(APPEND ${DEF_FILE} "CreatePlugin\n")
    file(APPEND ${DEF_FILE} "DestroyPlugin\n")
    # REMOVED LEGACY FUNCTIONS: EnginePluginInit, StudioPluginInit, PluginShutdown, PluginUpdate
    
    set_target_properties(ExamplePlugin PROPERTIES
        LINK_FLAGS "/DEF:${DEF_FILE}"
    )
endif()

# Compile definitions
target_compile_definitions(ExamplePlugin PRIVATE 
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    PCH_DISABLED
    DISABLE_PRECOMPILED_HEADERS
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Backup: Custom command to ensure directory and copy DLL
add_custom_command(TARGET ExamplePlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_TARGET_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ExamplePlugin>" "${PLUGIN_TARGET_DIR}/"
    COMMENT "Ensuring DLL is in: ${PLUGIN_TARGET_DIR}"
)

message(STATUS "ExamplePlugin configuration:")
message(STATUS "  Plugin will be built to: ${PLUGIN_TARGET_DIR}/ExamplePlugin.dll")
message(STATUS "  DEF file: ${DEF_FILE}")