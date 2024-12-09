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

void Events::ProcessEvents() {
    while (!eventQueue.empty()) {
        Event event = eventQueue.front();
        eventQueue.pop();

        // Handle event based on its type
        switch (event.type) {
        case EventType::QuitRequest: {
            Core.Quit();
        }
        case EventType::InferenceRequest: {
            std::cout << "Handling InferenceRequest event for Entity ID: " << event.entityID << std::endl;

            // Access the SDCPPSystem through EntityManager
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
            std::cout << "Handling Upscale event for Entity ID: " << event.entityID << std::endl;

            // Access the SDCPPSystem through EntityManager
            auto upscaleSystem = mgr.GetSystem<ECS::UpscaleSystem>();
            if (upscaleSystem) {
                std::cout << "UpscaleSystem is registered." << std::endl;
                upscaleSystem->QueueInference(event.entityID);
            } else {
                std::cerr << "SDCPPSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::ImageLoadRequest: {
            std::cout << "Handling ImageLoadRequest event for Entity ID: " << event.entityID << std::endl;
            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            if (imageSystem) {
                imageSystem->LoadImage(event.entityID);
            } else {
                std::cerr << "ImageSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::ImageSaveRequest: {
            std::cout << "Handling ImageSaveRequest event for Entity ID: " << event.entityID << std::endl;
            auto meshSystem = mgr.GetSystem<ECS::MeshSystem>();
            if (meshSystem) {
                // meshSystem->SaveMesh(event.entityID);
            } else {
                std::cerr << "ImageSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::MeshLoadRequest: {
            std::cout << "Handling ImageSaveRequest event for Entity ID: " << event.entityID << std::endl;
            auto meshSystem = mgr.GetSystem<ECS::MeshSystem>();
            if (meshSystem) {
                // meshSystem->LoadMesh(event.entityID);
            } else {
                std::cerr << "ImageSystem is not registered." << std::endl;
            }
            break;
        }
        case EventType::MeshSaveRequest: {
            
            break;
        }
        default:
            std::cerr << "Unknown event type" << std::endl; // Use cerr for errors
            break;
        }
    }
}

} // namespace ANI
