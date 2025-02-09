#include "MenuBar.hpp"
#include "../Events/Events.hpp"

namespace GUI {
static bool settingsWindowOpen = false;
static bool debugWindowOpen = false;

void ShowMenuBar(GLFWwindow *window) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                ANI::Event event;
                event.type = ANI::EventType::NewProject;
                ANI::Events::Ref().QueueEvent(event);
            };

            if (ImGui::MenuItem("Open")) {
                ANI::Event event;
                event.type = ANI::EventType::OpenProject;
                ANI::Events::Ref().QueueEvent(event);
            };
            ImGui::MenuItem("Save");
            if (ImGui::MenuItem("Exit")) {
                ANI::Event event;
                event.type = ANI::EventType::Quit;
                ANI::Events::Ref().QueueEvent(event);
            };
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::Separator();
            if (ImGui::MenuItem("Settings", nullptr, &settingsWindowOpen)) {
                ANI::Event event;
                event.type = settingsWindowOpen ? ANI::EventType::OpenSettings : ANI::EventType::CloseSettings;
                ANI::Events::Ref().QueueEvent(event);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            // View management here
            ImGui::Separator();
            if (ImGui::MenuItem("Debug", nullptr, &debugWindowOpen)) {
                ANI::Event event;
                event.type = debugWindowOpen ? ANI::EventType::OpenDebug : ANI::EventType::CloseDebug;
                ANI::Events::Ref().QueueEvent(event);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
} // namespace GUI