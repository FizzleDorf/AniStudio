#============================================================================
# AniStudioPlugin.cmake - Plugin build configuration template
#============================================================================
# This file should be placed in AniStudio/build/cmake/AniStudioPlugin.cmake
# and included by plugin CMakeLists.txt files

if(NOT DEFINED ANISTUDIO_BUILD_DIR)
    message(FATAL_ERROR "ANISTUDIO_BUILD_DIR must be defined by the build script")
endif()

if(NOT DEFINED ANISTUDIO_PLUGIN_DIR)
    message(FATAL_ERROR "ANISTUDIO_PLUGIN_DIR must be defined by the build script")
endif()

# Set up common plugin properties
function(setup_anistudio_plugin PLUGIN_NAME)
    # Set basic properties
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )
    
    # Platform-specific setup
    if(WIN32)
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${ANISTUDIO_PLUGIN_DIR}
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ANISTUDIO_PLUGIN_DIR}
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ANISTUDIO_PLUGIN_DIR}
            SUFFIX ".dll"
        )
        
        target_compile_definitions(${PLUGIN_NAME} PRIVATE
            "PLUGIN_EXPORTS"
            "ANI_CORE_API="
            "_CRT_SECURE_NO_WARNINGS"
        )
    else()
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${ANISTUDIO_PLUGIN_DIR}
            LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ANISTUDIO_PLUGIN_DIR}
            LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ANISTUDIO_PLUGIN_DIR}
            PREFIX "lib"
            SUFFIX ".so"
        )
        
        target_compile_definitions(${PLUGIN_NAME} PRIVATE
            "PLUGIN_EXPORTS"
        )
    endif()
    
    # Include directories
    target_include_directories(${PLUGIN_NAME} PRIVATE
        ${ANISTUDIO_BUILD_DIR}/../src
        ${ANISTUDIO_BUILD_DIR}/../src/base_plugins
        ${ANISTUDIO_BUILD_DIR}/../src/events
        ${ANISTUDIO_BUILD_DIR}/../src/gui
        ${ANISTUDIO_BUILD_DIR}/../src/gui/base
        ${ANISTUDIO_BUILD_DIR}/../src/utils
        ${ANISTUDIO_BUILD_DIR}/../src/gui_utils
        ${ANISTUDIO_BUILD_DIR}/../src/ecs
        ${ANISTUDIO_BUILD_DIR}/../src/ecs/base
        ${ANISTUDIO_BUILD_DIR}/../external/glm
        ${ANISTUDIO_BUILD_DIR}/../external/nlohmann_json
        ${ANISTUDIO_BUILD_DIR}/include
    )
    
    # Find and link AniStudio libraries
    find_library(ANISTUDIO_CORE_LIB
        NAMES AniStudioCore
        PATHS ${ANISTUDIO_BUILD_DIR}/lib ${ANISTUDIO_BUILD_DIR}
        NO_DEFAULT_PATH
    )
    
    if(ANISTUDIO_CORE_LIB)
        target_link_libraries(${PLUGIN_NAME} PRIVATE ${ANISTUDIO_CORE_LIB})
        message(STATUS "Linked AniStudioCore: ${ANISTUDIO_CORE_LIB}")
    else()
        message(WARNING "AniStudioCore library not found")
    endif()
    
    # Link common dependencies
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PLUGIN_NAME} PRIVATE OpenGL::GL)
    
    # Find and link semver
    find_package(semver QUIET)
    if(semver_FOUND)
        target_link_libraries(${PLUGIN_NAME} PRIVATE semver::semver)
        message(STATUS "Linked semver for plugin ${PLUGIN_NAME}")
    else()
        message(WARNING "semver not found - plugin versioning may not work properly")
    endif()
    
    # Try to find and link other common libraries
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        target_link_libraries(${PLUGIN_NAME} PRIVATE glfw)
    endif()
    
    find_package(GLEW QUIET)
    if(GLEW_FOUND)
        target_link_libraries(${PLUGIN_NAME} PRIVATE GLEW::GLEW)
    endif()
    
    # Set up PCH if available
    set(PCH_FILE "${ANISTUDIO_BUILD_DIR}/../src/pch.h")
    if(EXISTS "${PCH_FILE}")
        target_precompile_headers(${PLUGIN_NAME} PRIVATE "${PCH_FILE}")
        message(STATUS "Using PCH: ${PCH_FILE}")
    endif()
    
    message(STATUS "Plugin ${PLUGIN_NAME} configured for output to: ${ANISTUDIO_PLUGIN_DIR}")
endfunction()