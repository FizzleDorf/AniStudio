cmake_minimum_required(VERSION 3.8)

project(AniStudio)

# Set the directory containing the libraries
set(LIBRARY_DIR "${CMAKE_SOURCE_DIR}/external/libs")

# Set the full path to pch.h starting from the root directory
set(PCH_FILE "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/pch.h")

# Create a sources variable with a link to all cpp files to compile
set(SOURCES
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/App.cpp
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/pch.cpp
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Engine/Engine.cpp
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Timer/Timer.cpp
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Events/Events.cpp
)

# Manually specify the path to the GLEW library and include directories
set(GLEW_LIBRARY "${LIBRARY_DIR}/glew32s.lib")
set(GLEW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/includes")

# Manually specify the path to the GLFW3 library and include directories
set(GLFW3_LIBRARY "${LIBRARY_DIR}/glfw3.lib")
set(GLFW3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/includes")

# Add source to this project's executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${GLEW_INCLUDE_DIR}"
    "${GLFW3_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/external"  # Include directory for external libraries, source files, and headers
    "${CMAKE_SOURCE_DIR}/external/src"
    "${CMAKE_SOURCE_DIR}/external/libs"
    "${CMAKE_SOURCE_DIR}/external/includes"
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}"           # Include directory for project headers
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Engine" 
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Timer" 
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Events" 
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/ECS"
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Core"
)

# Add precompiled header file
target_precompile_headers(${PROJECT_NAME} PRIVATE "${PCH_FILE}")

# Link GLFW3 and GLEW libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    ${GLFW3_LIBRARY} 
    ${GLEW_LIBRARY}
)

# Set C++ standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# Include subdirectories containing additional source files or libraries
add_subdirectory(external/imgui-node-editor)
add_subdirectory(external/imgui)
