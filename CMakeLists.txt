cmake_minimum_required(VERSION 3.20)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the target compiler explicitly for all configurations
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2") # Adjust for your target architecture
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

find_package(OpenCV)
find_package(glew)
find_package(glfw3)
find_package(assimp)
find_package(ZLIB)

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
    AniStudio
    AniStudio/Engine
    AniStudio/Events
    AniStudio/Timer
    AniStudio/Gui
    AniStudio/utils
    AniStudio/ECS
    AniStudio/ECS/Base
    AniStudio/ECS/Systems
    AniStudio/ECS/Components
    AniStudio/ECS/Components/SDCPPComponents
    AniStudio/ECS/Components/ImageComponents
    AniStudio/ECS/Components/VideoComponents
    external/glm
    external/stable-diffusion.cpp
    # external/imgui
    # external/imgui/backends
    external/imgui_filedialog
    external/nlohmann_json
    external/imgui_nodes
    external/immvision
    ${Vulkan_INCLUDE_DIR}
)

# Find all source files
file(GLOB GUI_SOURCES AniStudio/Gui/*.cpp)
file(GLOB SYSTEM_SOURCES AniStudio/ECS/Systems/*.cpp)
file(GLOB UTILS_SOURCES AniStudio/utils/*.cpp)

# Add main executable
add_executable(${PROJECT_NAME}
    AniStudio/main.cpp
    AniStudio/pch.cpp
    AniStudio/Engine/Engine.cpp
    AniStudio/Events/Events.cpp
    AniStudio/Timer/Timer.cpp
    AniStudio/ECS/Base/EntityManager.cpp
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
