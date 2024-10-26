#pragma once
#include "Engine/Engine.hpp"
#include <GLFW/glfw3.h> // Assuming you're using GLFW for window handling
#include <functional>
#include <queue>

enum class EventType { InferenceRequest };

struct Event {
    EventType type;
};

namespace ANI {

class Events {
public:
    Events(const Events &) = delete;
    Events &operator=(const Events &) = delete;

    // Singleton reference
    static Events &Ref() {
        static Events reference;
        return reference;
    }

    void Poll();
    void Init(GLFWwindow *window);
    void QueueEvent(const Event &event);
    void ProcessEvents();

private:
    Events(); // Constructor is private for singleton pattern
    ~Events();
    std::queue<Event> eventQueue;

    // Static callback function for GLFW window close
    // static void WindowCloseCallback(GLFWwindow *window) { ANI::Core.Quit(); }
};

} // namespace ANI
