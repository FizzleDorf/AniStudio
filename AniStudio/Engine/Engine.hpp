#pragma once
#include "ECS.h"
#include "Gui/Guis.h"
#include "pch.h"
#include "TestDiffuseView.hpp"
#include <stable-diffusion.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
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

    inline const bool Run() const { return run; }
    inline GLFWwindow &Window() { return *window; }
    inline const int VideoWidth() const { return videoWidth; }
    inline const int VideoHeight() const { return videoHeight; }

private:
    Engine();
    bool run;
    GLFWwindow *window;
    int videoWidth;
    int videoHeight;
    TestDiffuseView *testDiffuseView; // Pointer to TestDiffusionView
    sd_ctx_t *sd_ctx;
};

extern Engine &Core;
} // namespace ANI
