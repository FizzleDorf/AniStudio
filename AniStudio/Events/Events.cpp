#include "Events.hpp"
#include "Engine/Engine.hpp"
#include <iostream>

namespace ANI {

Events::Events() {}

Events::~Events() {}

void Events::Init(GLFWwindow *window) {
    // Set the close callback to the static function that invokes Core.Quit()
    // glfwSetWindowCloseCallback(window, WindowCloseCallback);
}

void Events::QueueEvent(const Event &event) { eventQueue.push(event); }

void Events::Poll() {
    // Process all pending events
    ProcessEvents();
}

void Events::ProcessEvents() {
    while (!eventQueue.empty()) {
        Event event = eventQueue.front();
        eventQueue.pop();

        // Handle event based on its type
        switch (event.type) {
        case EventType::InferenceRequest:
            std::cout << "Handling InferenceRequest event" << std::endl;
            // Add event handling logic here
            break;
        default:
            std::cout << "Unknown event type" << std::endl;
            break;
        }
    }
}

} // namespace ANI
