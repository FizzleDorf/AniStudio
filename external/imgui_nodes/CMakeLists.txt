cmake_minimum_required(VERSION 3.20)
project(ImGui_Nodes)

# Add source files for ImGui
file(GLOB IMGUI_NODES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes/examples/application/source/*.cpp
)

add_library(ImGui_Nodes STATIC 
${IMGUI_NODES_SOURCES})

# Include directories for ImGui
target_include_directories(ImGui_Nodes PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes/examples/application/include
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes/examples/application/source
)
