# CMakeLists.txt for DiffusionAddon
cmake_minimum_required(VERSION 3.12)
project(DiffusionAddon VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# ============================================================================
# COLLECT SOURCE FILES
# ============================================================================

file(GLOB_RECURSE DIFFUSION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
)

# Add main addon implementation
list(APPEND DIFFUSION_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/DiffusionAddon.cpp")

# If there's GUI code for the addon
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gui")
    file(GLOB_RECURSE GUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp")
    list(APPEND DIFFUSION_SOURCES ${GUI_SOURCES})
endif()

# Create shared library
add_library(DiffusionAddon SHARED ${DIFFUSION_SOURCES})

# ============================================================================
# INCLUDE DIRECTORIES
# ============================================================================

target_include_directories(DiffusionAddon PRIVATE
    # Addon internal structure
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ecs
    ${CMAKE_CURRENT_SOURCE_DIR}/ecs/base
    ${CMAKE_CURRENT_SOURCE_DIR}/ecs/components
    ${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/sdcpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/media
    
    # GUI if present
    $<$<BOOL:${EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gui}>:${CMAKE_CURRENT_SOURCE_DIR}/gui>
    
    # Plugin interface from main project
    ${CMAKE_SOURCE_DIR}/src/aniengine/plugins
    ${CMAKE_SOURCE_DIR}/src/anistudio/plugins
    
    # ECS base from AniEngine (for EntityManager, types, etc.)
    ${CMAKE_SOURCE_DIR}/src/aniengine/ecs
    ${CMAKE_SOURCE_DIR}/src/aniengine/ecs/base
    
    # Asset manager
    ${CMAKE_SOURCE_DIR}/src/aniengine/asset_mgr
    
    # Main project utils (for FilePaths, etc.)
    ${CMAKE_SOURCE_DIR}/src/aniengine/utils
    ${CMAKE_SOURCE_DIR}/src/aniengine/utils/defaults
    
    # External dependencies
    ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
    ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    ${CMAKE_SOURCE_DIR}/external/stb_image
    ${CMAKE_SOURCE_DIR}/external/glm
)

# ============================================================================
# LINK LIBRARIES
# ============================================================================

# Find required dependencies for backend management
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)  # For SHA256 verification

target_link_libraries(DiffusionAddon PRIVATE
    # Link to AniEngine for ECS, AssetManager, utilities
    AniEngineCore
    
    # For downloading backends
    CURL::libcurl
    OpenSSL::Crypto  # For SHA256
    
    # System libraries for dynamic loading
    ${CMAKE_DL_LIBS}
    
    # DO NOT link stable-diffusion static lib - we load .dll/.so dynamically!
    # We only need headers at compile time for types
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(DiffusionAddon PRIVATE
        PLUGIN_API=__declspec\(dllexport\)
    )
    target_link_libraries(DiffusionAddon PRIVATE kernel32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(DiffusionAddon PRIVATE dl)
endif()

# Add miniz for ZIP extraction (header-only, bundled with project)
target_include_directories(DiffusionAddon PRIVATE
    ${CMAKE_SOURCE_DIR}/external/miniz
)

# ============================================================================
# OUTPUT DIRECTORIES - Match plugin hot reload structure
# ============================================================================

set(ADDON_NAME "DiffusionAddon")
set(ADDON_BASE_DIR "${CMAKE_BINARY_DIR}/addons/${ADDON_NAME}")
set(ADDON_STAGING_DIR "${ADDON_BASE_DIR}/staging")
set(ADDON_LIBS_DIR "${ADDON_BASE_DIR}/libs")

set_target_properties(DiffusionAddon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ADDON_STAGING_DIR}"
    
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${ADDON_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ADDON_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${ADDON_STAGING_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${ADDON_STAGING_DIR}"
    
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${ADDON_STAGING_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${ADDON_STAGING_DIR}"
)

# ============================================================================
# POST-BUILD SETUP
# ============================================================================

add_custom_command(TARGET DiffusionAddon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ADDON_BASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ADDON_STAGING_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ADDON_LIBS_DIR}"
    COMMENT "Creating DiffusionAddon directory structure"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LIBS_README.txt"
"Place stable-diffusion.cpp shared libraries here:\n\
\n\
Windows:\n\
  - stable-diffusion.dll\n\
  - ggml.dll\n\
\n\
Linux:\n\
  - libstable-diffusion.so\n\
  - libggml.so\n\
\n\
Download from: https://github.com/leejet/stable-diffusion.cpp/releases\n\
Or use script: scripts/download_sdcpp_libs.sh\n\
")

add_custom_command(TARGET DiffusionAddon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_BINARY_DIR}/LIBS_README.txt"
        "${ADDON_LIBS_DIR}/README.txt"
    COMMENT "Creating libs directory README"
)

message(STATUS "")
message(STATUS "=== DiffusionAddon Configuration ===")
message(STATUS "  Addon directory: ${ADDON_BASE_DIR}")
message(STATUS "  Staging (build): ${ADDON_STAGING_DIR}")
message(STATUS "  Libraries:       ${ADDON_LIBS_DIR}")
message(STATUS "")
message(STATUS "  Structure:")
message(STATUS "    - ecs/components/")
message(STATUS "    - ecs/systems/")
message(STATUS "    - utils/")
message(STATUS "    - gui/ (optional)")
message(STATUS "")
message(STATUS "  IMPORTANT:")
message(STATUS "    - This addon contains ALL SDCPP code")
message(STATUS "    - Download stable-diffusion.cpp libs to: ${ADDON_LIBS_DIR}")
message(STATUS "    - Hot reload enabled via PluginManager")
message(STATUS "")