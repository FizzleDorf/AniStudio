#pragma once
#include "ECS.h"
#include "GUI.h"
#include "../Engine/Engine.hpp"
#include <systems.h>
#include "filepaths.hpp"
#include <GLFW/glfw3.h>
#include <functional>
#include <queue>

namespace ANI {

enum class EventType {
    // Application
    Quit,
    NewProject,
    OpenProject,

    // Menu Views
    OpenSettings,
    CloseSettings,
    OpenDebug,
    CloseDebug,
    OpenConvert, 
    CloseConvert,
    OpenViews,
    CloseViews,
	OpenPlugins,
	ClosePlugins,

    // Diffusion
    InferenceRequest,
    Img2ImgRequest,
    UpscaleRequest,
    T2VInferenceRequest,
    ConvertToGGUF,
    
    // Queue Controls
    ClearInferenceQueue,
    PauseInference,
    ResumeInference,
    StopCurrentTask,

    // IO Events
    LoadImageEvent,
    SaveImageEvent,
    RemoveImageEvent,

    LoadVideoEvent,
    SaveVideoEvent
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

    // Static callback function for GLFW window close
    // static void WindowCloseCallback(GLFWwindow *window) { Engine::Ref().Quit();}
};

} // namespace ANI
