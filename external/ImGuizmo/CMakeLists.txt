# ImGuizmo CMakeLists.txt

# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.10)

# Create the ImGuizmo library
add_library(ImGuizmo STATIC
    GraphEditor.cpp
    ImCurveEdit.cpp
    ImGradient.cpp
    ImGuizmo.cpp
    ImSequencer.cpp
)

# Set include directories for ImGuizmo
target_include_directories(ImGuizmo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/imgui
)

# If ImGui is a separate target, you can use this instead:
# target_link_libraries(ImGuizmo PUBLIC ImGui)