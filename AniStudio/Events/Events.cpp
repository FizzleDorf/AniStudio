#include "../Engine/Engine.hpp"
#include "Events.hpp"
#include <iostream>

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
        case EventType::InferenceRequest: {
            std::cout << "Handling InferenceRequest event for Entity ID: " << event.entityID << '\n';

            auto sdcppSystem = mgr.GetSystem<ECS::SDCPPSystem>();
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

            auto upscaleSystem = mgr.GetSystem<ECS::UpscaleSystem>();
            if (upscaleSystem) {
                std::cout << "UpscaleSystem is registered." << std::endl;
                upscaleSystem->QueueInference(event.entityID);
            } else {
                std::cerr << "SDCPPSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::Interrupt: {
            std::cout << "Handling Interrupt event for SDCPPSystem" << '\n';

            auto sdcppSystem = mgr.GetSystem<ECS::SDCPPSystem>();
            if (sdcppSystem) {
                std::cout << "SDCPPSystem is registered." << std::endl;
                sdcppSystem->StopWorker();
            } else {
                std::cerr << "SDCPPSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::SaveImage: {
            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            imageSystem->SaveImage(event.entityID);
            break;
        }
        // case EventType::LoadImage:{
        //     auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
        //     imageSystem->AddImage(event.entityID);
        //     break;
        // }
        case EventType::SaveMesh: {
            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            // imageSystem->SaveImage(event.entityID);
            break;
        }
        case EventType::LoadMesh: {
            std::cout << "Handling ImageSaveRequest event for Entity ID: " << event.entityID << '\n';
            auto meshSystem = mgr.GetSystem<ECS::MeshSystem>();
            if (meshSystem) {
                // meshSystem->LoadMesh(event.entityID);
            } else {
                std::cerr << "ImageSystem is not registered." << std::endl;
            }
            break;
        }
        default:
            std::cerr << "Unknown event type" << std::endl; // Use cerr for errors
            break;
        }
    }
}

} // namespace ANI
