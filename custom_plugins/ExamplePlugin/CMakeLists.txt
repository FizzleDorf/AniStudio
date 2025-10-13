# Plugin CMakeLists.txt - RESTORED WORKING VERSION WITH VERSIONED DLL PATHS
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

# Get absolute paths for reliable directory references
get_filename_component(ANISTUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(CONAN_TOOLCHAIN_PATH "${ANISTUDIO_ROOT}/build/conan/conan_toolchain.cmake" ABSOLUTE)

# Include Conan toolchain from correct absolute path
include(${CONAN_TOOLCHAIN_PATH} OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(FATAL_ERROR "Conan toolchain not found at ${CONAN_TOOLCHAIN_PATH}. Please run the main project build first.")
endif()

# Find packages
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# Set external directory path
set(EXTERNAL_DIR "${ANISTUDIO_ROOT}/external")

# Add ImGui using absolute path
if(NOT TARGET ImGui AND EXISTS "${EXTERNAL_DIR}/imgui/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}/imgui" "${CMAKE_BINARY_DIR}/imgui")
    message(STATUS "Added ImGui for plugin from: ${EXTERNAL_DIR}/imgui")
elseif(TARGET ImGui)
    message(STATUS "ImGui target already available")
else()
    message(FATAL_ERROR "ImGui CMakeLists.txt not found at ${EXTERNAL_DIR}/imgui")
endif()

# Find or import AniEngineCore and AniStudioCore with absolute paths
set(MAIN_LIB_DIR "${ANISTUDIO_ROOT}/build/lib")

if(NOT TARGET AniEngineCore)
    find_library(AniEngineCore_LIB 
        NAMES AniEngineCore 
        PATHS "${MAIN_LIB_DIR}" 
        NO_DEFAULT_PATH)
    
    if(NOT AniEngineCore_LIB)
        message(FATAL_ERROR "AniEngineCore library not found at ${MAIN_LIB_DIR}. Please build the main project first.")
    endif()
    
    add_library(AniEngineCore SHARED IMPORTED)
    set_target_properties(AniEngineCore PROPERTIES
        IMPORTED_LOCATION "${AniEngineCore_LIB}"
        IMPORTED_IMPLIB "${AniEngineCore_LIB}"
    )
    message(STATUS "Imported AniEngineCore from: ${AniEngineCore_LIB}")
endif()

if(NOT TARGET AniStudioCore)
    find_library(AniStudioCore_LIB 
        NAMES AniStudioCore 
        PATHS "${MAIN_LIB_DIR}" 
        NO_DEFAULT_PATH)
    
    if(NOT AniStudioCore_LIB)
        message(FATAL_ERROR "AniStudioCore library not found at ${MAIN_LIB_DIR}. Please build the main project first.")
    endif()
    
    add_library(AniStudioCore SHARED IMPORTED)
    set_target_properties(AniStudioCore PROPERTIES
        IMPORTED_LOCATION "${AniStudioCore_LIB}"
        IMPORTED_IMPLIB "${AniStudioCore_LIB}"
    )
    message(STATUS "Imported AniStudioCore from: ${AniStudioCore_LIB}")
endif()

# Create the plugin DLL
add_library(ExamplePlugin SHARED
    ExamplePlugin.cpp
    # "${ANISTUDIO_ROOT}/src/anistudio/events/Events.cpp"
)

# Include paths with absolute references
target_include_directories(ExamplePlugin PRIVATE
    "${ANISTUDIO_ROOT}/src/aniengine/ecs"
    "${ANISTUDIO_ROOT}/src/aniengine/ecs/base"
    "${ANISTUDIO_ROOT}/src/aniengine/plugins"
    "${ANISTUDIO_ROOT}/src/anistudio/gui/base"
    "${ANISTUDIO_ROOT}/src/anistudio/plugins"
    "${ANISTUDIO_ROOT}/src/anistudio/core"
    # "${ANISTUDIO_ROOT}/src/anistudio/events"
    "${ANISTUDIO_ROOT}/src/anistudio"
    "${ANISTUDIO_ROOT}/src/aniengine/utils"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/imgui"
    "${EXTERNAL_DIR}/glm"
    "${ANISTUDIO_ROOT}/include"
)

# Link libraries
target_link_libraries(ExamplePlugin PRIVATE
    AniEngineCore
    AniStudioCore
    GLEW::GLEW
    glfw
    OpenGL::GL
    ImGui
)

# CORRECTED: Plugin directories for versioned DLL system
set(PLUGINS_BASE_DIR "${ANISTUDIO_ROOT}/build/plugins")
set(PLUGIN_FINAL_DIR "${PLUGINS_BASE_DIR}/ExamplePlugin")
set(PLUGIN_STAGING_DIR "${PLUGIN_FINAL_DIR}/staging")

# Create the directories
file(MAKE_DIRECTORY "${PLUGINS_BASE_DIR}")
file(MAKE_DIRECTORY "${PLUGIN_FINAL_DIR}")
file(MAKE_DIRECTORY "${PLUGIN_STAGING_DIR}")

# CRITICAL FOR VERSIONED HOT RELOAD: Build DLL directly to staging directory
set_target_properties(ExamplePlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PLUGIN_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PLUGIN_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PLUGIN_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PLUGIN_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PLUGIN_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PLUGIN_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PLUGIN_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${PLUGIN_STAGING_DIR}"
)

# DLL export settings for Windows
if(WIN32)
    set_target_properties(ExamplePlugin PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
    
    set(DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/ExamplePlugin.def")
    file(WRITE ${DEF_FILE} "EXPORTS\n")
    file(APPEND ${DEF_FILE} "CreatePlugin\n")
    file(APPEND ${DEF_FILE} "DestroyPlugin\n")
    
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

# Post-build verification
add_custom_command(TARGET ExamplePlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "===== VERSIONED HOT RELOAD BUILD SUCCESS ====="
    COMMAND ${CMAKE_COMMAND} -E echo "DLL built to: ${PLUGIN_STAGING_DIR}/$<TARGET_FILE_NAME:ExamplePlugin>"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin manager will detect this staging DLL"
    COMMAND ${CMAKE_COMMAND} -E echo "Hot reload will create versioned DLL in: ${PLUGIN_FINAL_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E echo "================================================"
    COMMENT "ExamplePlugin built for versioned hot reload"
)

# Manual promotion target (for testing)
add_custom_target(PromoteExamplePlugin
    COMMAND ${CMAKE_COMMAND} -E echo "Manually promoting plugin from staging..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PLUGIN_STAGING_DIR}/$<TARGET_FILE_NAME:ExamplePlugin>" 
        "${PLUGIN_FINAL_DIR}/ExamplePlugin_v1.dll"
    COMMAND ${CMAKE_COMMAND} -E remove "${PLUGIN_STAGING_DIR}/$<TARGET_FILE_NAME:ExamplePlugin>"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin promoted to ExamplePlugin_v1.dll - staging is now empty"
    DEPENDS ExamplePlugin
    COMMENT "Manually create versioned DLL from staging"
)

# Configuration summary
message(STATUS "")
message(STATUS "===== VERSIONED HOT RELOAD CONFIGURATION =====")
message(STATUS "Plugin Name: ExamplePlugin")
message(STATUS "Build target: ${PLUGIN_STAGING_DIR}/ExamplePlugin.dll")
message(STATUS "Versioned location: ${PLUGIN_FINAL_DIR}/ExamplePlugin_v*.dll") 
message(STATUS "")
message(STATUS "Versioned Hot Reload Flow:")
message(STATUS "  1. Build -> staging/ExamplePlugin.dll")
message(STATUS "  2. PluginManager detects new DLL in staging")
message(STATUS "  3. Create staging DLL as new versioned DLL (e.g., ExamplePlugin_v2.dll)")
message(STATUS "  4. Unload old versioned DLL, load new versioned DLL")
message(STATUS "  5. Clean up staging and optionally old versions")
message(STATUS "===============================================")
message(STATUS "")