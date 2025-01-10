#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "../Plugins/PluginManager.hpp"

#include "guis.h"
#include "GUI.h"
#include "ECS.h"

class ViewManager;

namespace ANI {

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 720;

class Engine {
public:
    static Engine &Ref() {
        static Engine instance;
        return instance;
    }

    ~Engine();

    void Init();
    void Update(const float deltatime);
    void Draw();
    void Quit();

    GLFWwindow *Window() const { return window; }
    bool Run() const { return run; }

private:
    Engine();
    
    bool run;
    GLFWwindow *window;
    int videoWidth;
    int videoHeight;
    double fpsSum = 0.0;
    int frameCount = 0;
    double timeElapsed = 0.0; // To track the elapsed time
};

void WindowCloseCallback(GLFWwindow *window);
extern Engine &Core;

} // namespace ANI

#endif // ENGINE_HPP
