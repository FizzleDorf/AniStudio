# AniStudio/CMakeLists.txt

# Set the full path to pch.h starting from the root directory
set(PCH_FILE "${CMAKE_SOURCE_DIR}/AniStudio/pch.h")

# Create a sources variable with a link to all cpp files to compile
set(SOURCES
    "${CMAKE_SOURCE_DIR}/AniStudio/main.cpp"
    "${CMAKE_SOURCE_DIR}/AniStudio/pch.cpp"
    "${CMAKE_SOURCE_DIR}/AniStudio/Engine/Engine.cpp"
    "${CMAKE_SOURCE_DIR}/AniStudio/Timer/Timer.cpp"
    "${CMAKE_SOURCE_DIR}/AniStudio/Events/Events.cpp"
)

# Add source to this project's executable
add_executable(AniStudio ${SOURCES})

# Include directories for external libraries and project headers
target_include_directories(AniStudio PRIVATE
    "${CMAKE_SOURCE_DIR}/external/includes"  # External libraries
    "${GLFW_INCLUDE_DIRS}"
    "${GLEW_INCLUDE_DIRS}"
    "${CMAKE_SOURCE_DIR}/AniStudio"           # Project headers
    "${CMAKE_SOURCE_DIR}/AniStudio/Engine" 
    "${CMAKE_SOURCE_DIR}/AniStudio/Timer" 
    "${CMAKE_SOURCE_DIR}/AniStudio/Events" 
    "${CMAKE_SOURCE_DIR}/AniStudio/ECS"
    "${CMAKE_SOURCE_DIR}/AniStudio/Core"
    "${CMAKE_SOURCE_DIR}/AniStudio/vk_engine" # Include vk_engine directory
    "${CMAKE_SOURCE_DIR}/external/imgui"  
)

# Add precompiled header file
target_precompile_headers(AniStudio PRIVATE "${PCH_FILE}")

# Link libraries
target_link_libraries(AniStudio 
    PRIVATE 
    imgui
    "${CMAKE_SOURCE_DIR}/external/libs/glfw3.lib"
    "${CMAKE_SOURCE_DIR}/external/libs/glew32s.lib"
)

add_subdirectory(vk_engine)
