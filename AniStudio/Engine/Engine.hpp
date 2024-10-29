#pragma once
#include "ECS.h"
#include "Gui/Guis.h"
#include "pch.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

namespace ANI {

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 720;

class Engine {
public:
    Engine(const Engine &) = delete;
    ~Engine();

    Engine &operator=(const Engine &) = delete;

    static Engine &Ref() {
        static Engine instance;
        return instance;
    }

    void Init();
    void Update();
    void Draw();
    void Quit();

    inline bool Run() const { return run; }
    inline GLFWwindow &Window() { return *window; }
    inline int VideoWidth() const { return videoWidth; }
    inline int VideoHeight() const { return videoHeight; }

private:
    Engine();
    bool run;
    GLFWwindow *window;
    int videoWidth;
    int videoHeight;
    ECS::EntityManager &mgr = ECS::EntityManager::Ref();
};

void WindowCloseCallback(GLFWwindow *window);
extern Engine &Core;

} // namespace ANI
