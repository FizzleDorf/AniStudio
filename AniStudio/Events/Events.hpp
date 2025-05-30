/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once
#include "ECS.h"
#include "../Gui/GUI.h"
#include "../Engine/Engine.hpp"
#include <systems.h>
#include "FilePaths.hpp"
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
