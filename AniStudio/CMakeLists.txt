# Set the full path to pch.h starting from the root directory
set(PCH_FILE "${CMAKE_SOURCE_DIR}/AniStudio/pch.h")

# Create a sources variable with a link to all cpp files to compile
set(SOURCES
    ${CMAKE_SOURCE_DIR}/AniStudio/App.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/pch.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/Engine/Engine.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/Timer/Timer.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/Events/Events.cpp
)

# Add source to this project's executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/external"  # Include directory for external libraries, source files, and headers
    "${CMAKE_SOURCE_DIR}/external/src"
    "${CMAKE_SOURCE_DIR}/external/libs"
    "${CMAKE_SOURCE_DIR}/external/includes"
    "${CMAKE_SOURCE_DIR}/AniStudio"           # Include directory for project headers
    "${CMAKE_SOURCE_DIR}/AniStudio/Engine" 
    "${CMAKE_SOURCE_DIR}/AniStudio/Timer" 
    "${CMAKE_SOURCE_DIR}/AniStudio/Events" 
    "${CMAKE_SOURCE_DIR}/AniStudio/ECS"
    "${CMAKE_SOURCE_DIR}/AniStudio/Core"
)

# Add precompiled header file
target_precompile_headers(${PROJECT_NAME} PRIVATE "${PCH_FILE}")

# Set C++ standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
