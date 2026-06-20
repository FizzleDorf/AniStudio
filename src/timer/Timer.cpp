#include "Timer.hpp"
#include <GLFW/glfw3.h>

namespace ANI {
    T_Timer::T_Timer() : m_deltaTime(0.0f), m_lastFrame(0.0f) {

    }

    T_Timer::~T_Timer() {

    }

    void T_Timer::Tick() {
        m_deltaTime = glfwGetTime() - m_lastFrame;
        m_lastFrame = glfwGetTime();
    }

    void T_Timer::Init() {

    }
}
