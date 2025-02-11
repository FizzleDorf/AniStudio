#include "Events.hpp"
#include <iostream>
#include "guis.h"

namespace ANI {

Events::Events() {}

Events::~Events() {}

void Events::Init(GLFWwindow *window) {
    // Set the close callback to the static function that invokes Engine::Quit()
    glfwSetWindowCloseCallback(window, WindowCloseCallback);
}

void Events::QueueEvent(const Event &event) { eventQueue.push(event); }

void Events::Poll() {
    // Poll and handle events (inputs, window resize, etc.)
    glfwPollEvents();

    // Process all pending events after polling
    ProcessEvents();
}
static size_t debugID = 0;
static size_t settingsID = 0;
static size_t viewsID = 0;

// Handle events based on its EventType
void Events::ProcessEvents() {
    while (!eventQueue.empty()) {
        Event event = eventQueue.front();
        eventQueue.pop();

        switch (event.type) {
        case EventType::Quit: {
            Core.Quit();
            break;
        }
        case EventType::OpenSettings: {
            auto &vMgr = Core.GetViewManager();
            ViewListID id = vMgr.CreateView();
            vMgr.AddView<SettingsView>(id, SettingsView(Core.GetEntityManager()));
            vMgr.GetView<SettingsView>(id).Init();
            settingsID = id;
            break;
        }
        case EventType::CloseSettings: {
            auto &vMgr = Core.GetViewManager();
            auto views = vMgr.GetAllViews();

                if (vMgr.HasView<SettingsView>(settingsID)) {
                vMgr.DestroyView(settingsID);
                }
            
            break;
        }
        case EventType::OpenDebug: {
            auto &vMgr = Core.GetViewManager();
            ViewListID id = vMgr.CreateView();
            vMgr.AddView<DebugView>(id, DebugView(Core.GetEntityManager()));
            vMgr.GetView<DebugView>(id).Init();
            debugID = id;
            break;
        }
        case EventType::CloseDebug: {
            auto &vMgr = Core.GetViewManager();
            auto views = vMgr.GetAllViews();
            for (auto view : views) {
                if (vMgr.HasView<DebugView>(debugID)) {
                    vMgr.DestroyView(debugID);
                }
            }
            break;
        }
        case EventType::OpenViews: {
            auto &vMgr = Core.GetViewManager();
            ViewListID id = vMgr.CreateView();
            vMgr.AddView<ViewManagerView>(id, ViewManagerView(Core.GetEntityManager(), vMgr));
            vMgr.GetView<ViewManagerView>(id).Init();
            viewsID = id;
            break;
        }
        case EventType::CloseViews: {
            auto &vMgr = Core.GetViewManager();
            auto views = vMgr.GetAllViews();
            for (auto view : views) {
                if (vMgr.HasView<ViewManagerView>(viewsID)) {
                    vMgr.DestroyView(viewsID);
                }
            }
            break;
        }
        case EventType::InferenceRequest: {
            std::cout << "Handling InferenceRequest event for Entity ID: " << event.entityID << '\n';
            
            auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
            if (sdcppSystem) {
                std::cout << "SDCPPSystem is registered." << std::endl;
                sdcppSystem->QueueInference(event.entityID);
            } else {
                std::cerr << "SDCPPSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::UpscaleRequest: {
            std::cout << "Handling Upscale event for Entity ID: " << event.entityID << '\n';

            auto upscaleSystem = Core.GetEntityManager().GetSystem<ECS::UpscaleSystem>();
            if (upscaleSystem) {
                std::cout << "UpscaleSystem is registered." << std::endl;
                upscaleSystem->QueueInference(event.entityID);
            } else {
                std::cerr << "UpscaleSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::ConvertToGGUF: {
            std::cout << "Handling Convert event for Entity ID: " << event.entityID << '\n';

            auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
            if (sdcppSystem) {
                std::cout << "SDCPPSystem is registered." << std::endl;
                sdcppSystem->QueueConversion(event.entityID);
            } else {
                std::cerr << "SDCPPSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::Interrupt: {
            std::cout << "Handling Interrupt event for SDCPPSystem" << '\n';

            auto sdcppSystem = Core.GetEntityManager().GetSystem<ECS::SDCPPSystem>();
            if (sdcppSystem) {
                std::cout << "SDCPPSystem is registered." << std::endl;
                sdcppSystem->StopWorker();
            } else {
                std::cerr << "SDCPPSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::SaveImageEvent: {
            auto imageSystem = Core.GetEntityManager().GetSystem<ECS::ImageSystem>();
            imageSystem->SaveImage(event.entityID);
            break;
        }
         case EventType::LoadImageEvent:{
            auto imageSystem = Core.GetEntityManager().GetSystem<ECS::ImageSystem>();
             // imageSystem->AddImage(event.entityID);
             break;
         }
        default:
            std::cerr << "Unknown event type" << std::endl; // Use cerr for errors
            break;
        }
    }
}

} // namespace ANI
