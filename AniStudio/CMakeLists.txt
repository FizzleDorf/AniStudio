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
    "${GLFW_INCLUDE_DIRS}"
    "${GLEW_INCLUDE_DIRS}"
    "${VULKAN_INCLUDE_DIRS}"
    "${CMAKE_SOURCE_DIR}/AniStudio"           # Project headers
    "${CMAKE_SOURCE_DIR}/AniStudio/Engine" 
    "${CMAKE_SOURCE_DIR}/AniStudio/Timer" 
    "${CMAKE_SOURCE_DIR}/AniStudio/Events" 
    "${CMAKE_SOURCE_DIR}/AniStudio/ECS"
    "${CMAKE_SOURCE_DIR}/AniStudio/Core"
)

# Add precompiled header file
target_precompile_headers(AniStudio PRIVATE "${PCH_FILE}")

# Define GLEW_STATIC for static linking
target_compile_definitions(AniStudio PRIVATE GLEW_STATIC)

# Link libraries (example with imgui, GLFW, GLEW, Vulkan, and OpenGL)
target_link_libraries(AniStudio 
    PRIVATE 
    imgui
    ${GLFW_LIBRARIES} 
    ${GLEW_LIBRARIES}
    ${VULKAN_LIBRARIES}  
    ${OPENGL_LIBRARIES}
)

# Set C++ standard
set_property(TARGET AniStudio PROPERTY CXX_STANDARD 20)
