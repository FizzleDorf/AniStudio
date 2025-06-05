# Plugin CMakeLists.txt for AniStudio
cmake_minimum_required(VERSION 3.12)

# Set C++ standard to match AniStudio
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directory for plugins
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# Function to create a plugin
function(add_anistudio_plugin PLUGIN_NAME)
    set(PLUGIN_SOURCES ${ARGN})
    
    # Create the plugin as a shared library
    add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES})
    
    # Ensure AniStudioCore is built first
    add_dependencies(${PLUGIN_NAME} AniStudioCore)
    
    # Link with AniStudioCore static library
    target_link_libraries(${PLUGIN_NAME} PRIVATE AniStudioCore)
    
    # Set plugin-specific properties
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        PREFIX ""  # Remove lib prefix on Unix systems
        SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        # Set output directory for plugins
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins
    )
    
    # Platform-specific settings
    if(WIN32)
        target_compile_definitions(${PLUGIN_NAME} PRIVATE
            "ANI_CORE_API="
            "PLUGIN_API=__declspec(dllexport)"
        )
        # Ensure proper symbol export on Windows
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS OFF  # We want explicit exports only
        )
    else()
        target_compile_definitions(${PLUGIN_NAME} PRIVATE 
            "ANI_CORE_API="
            "PLUGIN_API=__attribute__((visibility(\"default\")))"
        )
        # Set visibility for Linux/Mac
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            CXX_VISIBILITY_PRESET hidden
        )
    endif()
    
    message(STATUS "Configured plugin: ${PLUGIN_NAME}")
endfunction()

# Example plugin - FIXED: Use the correct filename
add_anistudio_plugin(ExamplePlugin
    ExamplePlugin.cpp
)

# Create a target to build all plugins
add_custom_target(all_plugins)

# Add dependencies for plugins that exist
if(TARGET ExamplePlugin)
    add_dependencies(all_plugins ExamplePlugin)
endif()

# Install targets for existing plugins
set(PLUGIN_TARGETS "")

if(TARGET ExamplePlugin)
    list(APPEND PLUGIN_TARGETS ExamplePlugin)
endif()

# Install plugins to the correct directory
if(PLUGIN_TARGETS)
    install(TARGETS ${PLUGIN_TARGETS}
        LIBRARY DESTINATION plugins
        RUNTIME DESTINATION plugins
    )
endif()