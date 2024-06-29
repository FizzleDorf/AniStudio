# Submodules/imgui/CMakeLists.txt

file(GLOB IMGUI_SRC
    *.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

add_library(ImGui STATIC ${IMGUI_SRC} "examples/example_glfw_opengl3/canvas-example/canvas-example.cpp")
target_include_directories(ImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
