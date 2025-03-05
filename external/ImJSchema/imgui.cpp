// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important to understand: SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <../res/bindings/imgui_impl_sdl2.cpp>
#include <../res/bindings/imgui_impl_sdlrenderer2.cpp>

#include <src/imgui_demo.cpp>
#include <src/imgui_widgets.cpp>
#include <src/imgui_draw.cpp>
#include <src/imgui_tables.cpp>
#include <src/imgui.cpp>
#include <imgui_stdlib.cpp>


