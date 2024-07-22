# external/imgui/CMakeLists.txt

cmake_minimum_required(VERSION 3.8)
project(imgui)

file(GLOB imgui_sources
    "*.cpp"
    "*.h"
)

add_library(imgui STATIC ${imgui_sources})

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
