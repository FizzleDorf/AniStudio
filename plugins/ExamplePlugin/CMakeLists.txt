cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)
project(ExamplePlugin VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Required parameters from the build script
if(NOT DEFINED ANISTUDIO_BUILD_DIR)
    message(FATAL_ERROR "ANISTUDIO_BUILD_DIR must be defined. Use the build script!")
endif()

if(NOT DEFINED ANISTUDIO_PLUGIN_DIR)
    message(FATAL_ERROR "ANISTUDIO_PLUGIN_DIR must be defined. Use the build script!")
endif()

message(STATUS "ExamplePlugin configuration:")
message(STATUS "  AniStudio build dir: ${ANISTUDIO_BUILD_DIR}")
message(STATUS "  Plugin output dir: ${ANISTUDIO_PLUGIN_DIR}")

# Include Conan toolchain from AniStudio build directory
set(CONAN_TOOLCHAIN_FILE "${ANISTUDIO_BUILD_DIR}/conan/conan_toolchain.cmake")
if(EXISTS "${CONAN_TOOLCHAIN_FILE}")
    include("${CONAN_TOOLCHAIN_FILE}")
    message(STATUS "Found and included Conan toolchain: ${CONAN_TOOLCHAIN_FILE}")
else()
    message(FATAL_ERROR "Conan toolchain not found at: ${CONAN_TOOLCHAIN_FILE}")
endif()

# Now find packages using Conan's configuration
find_package(OpenCV REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Create the plugin as a shared library
add_library(${PROJECT_NAME} SHARED
    ExamplePlugin.cpp
)

# Set plugin properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Platform-specific plugin setup
if(WIN32)
    # Windows DLL setup
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${ANISTUDIO_PLUGIN_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ANISTUDIO_PLUGIN_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ANISTUDIO_PLUGIN_DIR}
        SUFFIX ".dll"
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "PLUGIN_API=__declspec(dllexport)"
        "ANI_CORE_API="
        "_CRT_SECURE_NO_WARNINGS"  # Disable strcpy warnings
    )
else()
    # Linux/Mac shared library setup  
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${ANISTUDIO_PLUGIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ANISTUDIO_PLUGIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ANISTUDIO_PLUGIN_DIR}
        PREFIX "lib"
        SUFFIX ".so"
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "PLUGIN_API=__attribute__((visibility(\"default\")))"
        "ANI_CORE_API="
    )
endif()

# Add ALL the include directories that the plugin needs
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CONAN_INCLUDE_DIRS}
    ${ANISTUDIO_BUILD_DIR}/../AniStudio
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/ECS
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/ECS/Base
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/ECS/Systems
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/ECS/Components
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/ECS/Components/SDCPPComponents
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/ECS/Components/ImageComponents
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/ECS/Components/VideoComponents
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/GUI
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/GUI/Base
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/Plugins
    ${ANISTUDIO_BUILD_DIR}/../AniStudio/utils
    ${ANISTUDIO_BUILD_DIR}/../external/imgui
    ${ANISTUDIO_BUILD_DIR}/../external/nlohmann_json
    ${ANISTUDIO_BUILD_DIR}/../external/glm
    ${ANISTUDIO_BUILD_DIR}/../external/stable-diffusion.cpp
)

# Link to external libraries from the main project build
# First, try to find the built libraries in the AniStudio build directory
set(IMGUI_LIB_PATH "${ANISTUDIO_BUILD_DIR}/lib")
set(IMGUI_BIN_PATH "${ANISTUDIO_BUILD_DIR}/bin")

# Link to the ImGui library built with AniStudio
if(WIN32)
    # On Windows, look for the .lib file
    find_library(IMGUI_LIBRARY 
        NAMES ImGui
        PATHS ${IMGUI_LIB_PATH}
        NO_DEFAULT_PATH
    )
    
    if(IMGUI_LIBRARY)
        message(STATUS "Found ImGui library: ${IMGUI_LIBRARY}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${IMGUI_LIBRARY})
    else()
        message(WARNING "ImGui library not found in ${IMGUI_LIB_PATH}")
    endif()
else()
    # On Linux/Mac, look for the .a or .so file
    find_library(IMGUI_LIBRARY 
        NAMES ImGui libImGui
        PATHS ${IMGUI_LIB_PATH}
        NO_DEFAULT_PATH
    )
    
    if(IMGUI_LIBRARY)
        message(STATUS "Found ImGui library: ${IMGUI_LIBRARY}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${IMGUI_LIBRARY})
    else()
        message(WARNING "ImGui library not found in ${IMGUI_LIB_PATH}")
    endif()
endif()

# Also try to find other AniStudio libraries
find_library(ANISTUDIO_CORE_LIBRARY 
    NAMES AniStudioCore
    PATHS ${IMGUI_LIB_PATH}
    NO_DEFAULT_PATH
)

if(ANISTUDIO_CORE_LIBRARY)
    message(STATUS "Found AniStudioCore library: ${ANISTUDIO_CORE_LIBRARY}")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ANISTUDIO_CORE_LIBRARY})
else()
    message(WARNING "AniStudioCore library not found in ${IMGUI_LIB_PATH}")
endif()

# Link Conan libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${CONAN_LIBS}
    GLEW::GLEW
    glfw
    opencv::opencv
)

# Set up PCH after include directories are configured
set(PCH_FILE "${ANISTUDIO_BUILD_DIR}/../AniStudio/pch.h")
if(EXISTS "${PCH_FILE}")
    target_precompile_headers(${PROJECT_NAME} PRIVATE "${PCH_FILE}")
    message(STATUS "Using PCH file: ${PCH_FILE}")
else()
    message(WARNING "PCH file not found at ${PCH_FILE}, building without PCH")
endif()

# Debug information
message(STATUS "ExamplePlugin configured successfully")
message(STATUS "Output will be: ${ANISTUDIO_PLUGIN_DIR}/${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
message(STATUS "Looking for libraries in: ${IMGUI_LIB_PATH}")

# Print all found libraries for debugging
get_target_property(LINKED_LIBS ${PROJECT_NAME} LINK_LIBRARIES)
message(STATUS "Plugin will link to: ${LINKED_LIBS}")