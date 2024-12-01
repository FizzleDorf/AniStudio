#pragma once
#include "ECS.h"
#include <GLFW/glfw3.h>
#include <functional>
#include <queue>

namespace ANI {

enum class EventType { 
    // Diffusion
    InferenceRequest,
    UpscaleRequest,
    I2IInferenceRequest,
    T2VInferenceRequest,

    // IO Events
    ImageLoadRequest,
    ImageSaveRequest
};

struct Event {
    EventType type;
    ECS::EntityID entityID;
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
    ECS::EntityManager &mgr = ECS::EntityManager::Ref();

    // Static callback function for GLFW window close
    static void WindowCloseCallback(GLFWwindow *window) {
        Engine::Ref().Quit();
    }
};

} // namespace ANI
