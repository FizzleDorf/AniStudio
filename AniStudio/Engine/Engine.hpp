#pragma once

#include "Gui/Guis.h" // Retain the custom Gui classes
#include "VulkanContext.hpp"
#include <GLFW/glfw3.h>

namespace ANI {
class Engine {
public:
    Engine();
    ~Engine();

    Engine &operator=(const Engine &) = delete;

    static Engine &Ref() {
        static Engine instance;
        return instance;
    }

    void Init();
    void Update();
    void Render();
    void Quit() { isRunning = false; }

    inline const bool Run() const { return isRunning; }
    inline GLFWwindow &Window() { return *window; }
    inline const int VideoWidth() const { return videoWidth; }
    inline const int VideoHeight() const { return videoHeight; }

private:
    bool isRunning;
    GLFWwindow *window;
    VulkanContext vulkanContext;

    // GUI components
    GuiDiffusion diffusionView;
    GuiSettings settingsView;

    int videoWidth = 1200;
    int videoHeight = 720;
};

// Declare the singleton Core engine globally
extern Engine &Core;

} // namespace ANI
