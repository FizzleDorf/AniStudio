#pragma once
#include "ECS.h"
#include <GLFW/glfw3.h>
#include <functional>
#include <queue>

namespace ANI {

class Engine; // Forward declaration of Engine

enum class EventType { InferenceRequest };

struct Event {
    EventType type;
    EntityID entityID;
};

class Events {
public:
    ~Events();
    Events(const Events &) = delete;
    Events &operator=(const Events &) = delete;

    // Singleton reference
    static Events &Ref() {
        static Events instance; // Changed reference to instance for clarity
        return instance;
    }

    void Poll();
    void Init(GLFWwindow *window);
    void QueueEvent(const Event &event);
    void ProcessEvents();

private:
    Events(); // Constructor is private for singleton pattern
    std::queue<Event> eventQueue;

    // Static callback function for GLFW window close
    static void WindowCloseCallback(GLFWwindow *window) {
        ANI::Engine::Ref().Quit();
    }
};

} // namespace ANI
