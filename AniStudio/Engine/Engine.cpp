#include "Engine.hpp"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <iostream>

using namespace ECS;
using namespace ANI;

namespace ANI {
// Singleton instance of the engine
Engine &Core = Engine::Ref();

Engine::Engine() : isRunning(true), window(nullptr) {}

Engine::~Engine() {
    // Properly shut down Vulkan and clean up resources
    vulkanContext.Cleanup();
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

// Initialize GLFW and Vulkan
void Engine::Init() {
    // Initialize GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    std::cout << "GLFW Initialized" << std::endl;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context
    window = glfwCreateWindow(videoWidth, videoHeight, "AniStudio", nullptr, nullptr);

    if (!window) {       // Check if the window was created successfully
        glfwTerminate(); // Terminate GLFW if window creation failed
        throw std::runtime_error("Failed to create GLFW window");
    }
    std::cout << "AniStudio Window Created" << std::endl;
    vulkanContext.Init(window); // Initialize Vulkan and ImGui
}

void Engine::Update() {
    while (!glfwWindowShouldClose(window) && isRunning) {
        glfwPollEvents();
        Render();
    }
}

void Engine::Render() {
    // Start new ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // GUI Components Rendering
    ImGui::Begin("Main Menu");
    ImGui::Text("Welcome to the Main Menu!");

    // Render custom GUI views
    diffusionView.Render();
    settingsView.Render();
    ImGui::End();

    // Render ImGui data
    ImGui::Render();
    vulkanContext.RenderFrame(ImGui::GetDrawData());
    vulkanContext.PresentFrame();
}
} // namespace ANI
