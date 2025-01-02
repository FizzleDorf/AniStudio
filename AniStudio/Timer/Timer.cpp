#include "Timer.hpp"
#include <glfw/glfw3.h>

namespace ANI {
    T_Timer::T_Timer() : deltaTime(0.0f), lastFrame(0.0f) {

    }

    T_Timer::~T_Timer() {

    }

    void T_Timer::Tick() {
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
    }

    void T_Timer::Init() {

    }
}
