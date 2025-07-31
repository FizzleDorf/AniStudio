# CMakeLists.txt for ExamplePlugin - SIMPLE VERSION
cmake_minimum_required(VERSION 3.12)
project(ExamplePlugin)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)

# Create the plugin
add_library(ExamplePlugin SHARED ExamplePlugin.cpp)

# ONLY the includes we actually need
target_include_directories(ExamplePlugin PRIVATE
    # Plugin API
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    
    # ECS basics
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/aniengine/ecs/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/aniengine/ecs
    
    # JSON
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/nlohmann_json
)

# Plugin export
target_compile_definitions(ExamplePlugin PRIVATE
    PLUGIN_API=__declspec\(dllexport\)
)

# Output to plugins directory
set_target_properties(ExamplePlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../build/plugins
    PREFIX ""
)