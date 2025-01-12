cmake_minimum_required(VERSION 3.20)

# Build options
option(BUILD_ANISTUDIO "Build the AniStudio application" ON)
option(BUILD_PLUGINS "Build AniStudio plugins" ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the target compiler explicitly for all configurations
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2") # Adjust for target architecture
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64") # For 64-bit architecture
endif()

# Set the target architecture explicitly for all configurations
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(CMAKE_SIZEOF_VOID_P 8) # Ensure 64-bit
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
    set(CMAKE_SIZEOF_VOID_P 4) # Ensure 32-bit
endif()

# Include Conan-generated cmake files
include(build/generators/conan_toolchain.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

project(AniStudio CXX)

if(BUILD_ANISTUDIO)
    # Required packages for AniStudio
    find_package(OpenCV REQUIRED)
    find_package(glew REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(assimp REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)

    # Set PCH file
    set(PCH_FILE "${CMAKE_SOURCE_DIR}/AniStudio/pch.h")

    # Define the shader source directory
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/Anistudio/shaders")

    # Copy the shaders to the build directory
    file(COPY ${SHADER_DIR}/basic.vert ${SHADER_DIR}/basic.frag DESTINATION ${CMAKE_BINARY_DIR}/shaders)

    # External libraries
    add_subdirectory(external/stable-diffusion.cpp)
    add_subdirectory(external/imgui)
    add_subdirectory(external/imgui_nodes)
    add_subdirectory(external/stb_image)
    add_subdirectory(external/imgui_filedialog)
    add_subdirectory(external/ImGuizmo)

    # Include directories
    include_directories(
        ${CONAN_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/AniStudio
        ${CMAKE_SOURCE_DIR}/AniStudio/Plugins
        ${CMAKE_SOURCE_DIR}/AniStudio/Engine
        ${CMAKE_SOURCE_DIR}/AniStudio/Events
        ${CMAKE_SOURCE_DIR}/AniStudio/Timer
        ${CMAKE_SOURCE_DIR}/AniStudio/GUI
        ${CMAKE_SOURCE_DIR}/AniStudio/GUI/Base
        ${CMAKE_SOURCE_DIR}/AniStudio/utils
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Systems
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/SDCPPComponents
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/ImageComponents
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/VideoComponents
        ${CMAKE_SOURCE_DIR}/external/glm
        ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
        ${CMAKE_SOURCE_DIR}/external/nlohmann_json
        ${CMAKE_SOURCE_DIR}/external/imgui_nodes
        ${CMAKE_SOURCE_DIR}/external/immvision
        ${Vulkan_INCLUDE_DIR}
    )

    # Find all source files
    file(GLOB GUI_SOURCES AniStudio/GUI/*.cpp)
    file(GLOB SYSTEM_SOURCES AniStudio/ECS/Systems/*.cpp)
    file(GLOB UTILS_SOURCES AniStudio/utils/*.cpp)

    # Add main executable
    add_executable(${PROJECT_NAME}
        AniStudio/main.cpp
        AniStudio/pch.cpp
        AniStudio/Engine/Engine.cpp
        AniStudio/Events/Events.cpp
        AniStudio/Timer/Timer.cpp
        ${GUI_SOURCES}
        ${SYSTEM_SOURCES}
        ${UTILS_SOURCES}
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)
    link_directories(${CONAN_LIB_DIRS})

    # Link libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CONAN_LIBS}   
        stable-diffusion
        ImGui
        ImGui_Nodes
        stb_image
        ImGuiFileDialog 
        ImGuizmo
        opencv::opencv 
        glfw
        GLEW::GLEW 
        assimp::assimp 
        ZLIB::ZLIB
        PNG::PNG
    )

    target_precompile_headers(${PROJECT_NAME} PRIVATE ${PCH_FILE})
endif()

# Build plugins if enabled
if(BUILD_PLUGINS)
    # Only include headers needed for plugins
    if(NOT BUILD_ANISTUDIO)
        add_subdirectory(external/stb_image)
        add_subdirectory(external/stable-diffusion.cpp)
        include_directories(
            ${CMAKE_SOURCE_DIR}/AniStudio
            ${CMAKE_SOURCE_DIR}/AniStudio/ECS
            ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
            ${CMAKE_SOURCE_DIR}/AniStudio/GUI/Base
            ${CMAKE_SOURCE_DIR}/external/nlohmann_json
            ${CMAKE_SOURCE_DIR}/external/stb_image
            ${CMAKE_SOURCE_DIR}/external/imgui
        )
    endif()
    add_subdirectory(plugins)
endif()