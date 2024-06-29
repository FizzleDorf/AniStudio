cmake_minimum_required(VERSION 3.8)

project(ImGuiExample)

# Find the required packages
find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED) # Use GLFW instead of GLFW3
find_package(GLEW REQUIRED)

# Add the executable
add_executable(ImGuiExample main.cpp)

# Include directories
target_include_directories(ImGuiExample PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/Submodules/imgui
)

# Link libraries
target_link_libraries(ImGuiExample PRIVATE
    ImGui
    OpenGL::GL
    glfw # Link with GLFW instead of GLFW3
    GLEW::GLEW
)
