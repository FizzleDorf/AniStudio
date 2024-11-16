cmake_minimum_required(VERSION 3.20)
project(ImGui)

# Add source files for ImGui
file(GLOB IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ## ${CMAKE_CURRENT_SOURCE_DIR}/backends/*.cpp
)

add_library(ImGui STATIC ${IMGUI_SOURCES})

# Include directories for ImGui
target_include_directories(ImGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ## ${CMAKE_CURRENT_SOURCE_DIR}/backends
)
