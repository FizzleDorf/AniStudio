cmake_minimum_required(VERSION 3.8)

project(AniStudio)

# Set the directory containing the libraries
set(LIBRARY_DIR "${CMAKE_SOURCE_DIR}/Libraries/libs")

# Set the full path to pch.h starting from the root directory
set(PCH_FILE "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/pch.h")

# Create a sources variable with a link to all cpp files to compile
set(SOURCES
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/App.cpp
    #${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/pch.cpp
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Engine/Engine.cpp
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Timer/Timer.cpp
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/Events/Events.cpp
)

# Find and link GLEW
find_library(GLEW_LIBRARY
    NAMES glew32s
    HINTS ${LIBRARY_DIR}
)
if(GLEW_LIBRARY)
    message(STATUS "Found GLEW: ${GLEW_LIBRARY}")
else()
    message(FATAL_ERROR "GLEW library not found")
endif()

# Find and link GLFW3
find_library(GLFW3_LIBRARY
    NAMES glfw3
    HINTS ${LIBRARY_DIR}
)
if(GLFW3_LIBRARY)
    message(STATUS "Found GLFW3: ${GLFW3_LIBRARY}")
else()
    message(FATAL_ERROR "GLFW3 library not found")
endif()

# Add source to this project's executable.
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/Libraries/includes"  # Include directory for external libraries
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

# Include subdirectory containing additional source files
# add_subdirectory("${CMAKE_SOURCE_DIR}/${PROJECT_NAME}")
