cmake_minimum_required(VERSION 3.20)
project(ExamplePlugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Conan-generated cmake files
include(${CMAKE_SOURCE_DIR}/build/generators/conan_toolchain.cmake)

find_package(Vulkan REQUIRED)
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)

# Create shared library
add_library(${PROJECT_NAME} SHARED
    ExamplePlugin.cpp
)

file(GLOB IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/external/imgui/*.cpp
)

target_sources(${PROJECT_NAME} PRIVATE 
    ${IMGUI_SOURCES}
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base/EntityManager.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/GUI/Base/ViewManager.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/AniStudio
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/ImageComponents
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/VideoComponents
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Systems
    ${CMAKE_SOURCE_DIR}/AniStudio/GUI
    ${CMAKE_SOURCE_DIR}/AniStudio/utils
    ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${Vulkan_INCLUDE_DIR}

)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    GLEW::GLEW
    stb_image
    Vulkan::Vulkan
)
# Set PCH file
set(PCH_FILE "${CMAKE_SOURCE_DIR}/AniStudio/pch.h")

# Set output directory to plugins folder
set_target_properties(${PROJECT_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)