cmake_minimum_required(VERSION 3.26)

# CREATE PROJECT
project(ImNodeFlow)



add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

# SET SOURCE FILES FOR PROJECT
file(GLOB_RECURSE _HDRS "include/*.h")
file(GLOB_RECURSE _SRCS "src/*.cpp" "src/*.h" "src/*.inl")
file(GLOB IMGUI_HDRS "../imgui/*.h")
file(GLOB IMGUI_SRCS "../imgui/*.cpp")
# CREATE LIBRARY FROM SOURCE_FILES
add_library(ImNodeFlow ${_SRCS} ${_HDRS} ${IMGUI_SRCS} ${IMGUI_HDRS})



# FIND DEPENDENCIES
include_directories(${CMAKE_SOURCE_DIR}/external/imgui)

# PREP TO USE "#include <>"
target_include_directories(ImNodeFlow PUBLIC include)
