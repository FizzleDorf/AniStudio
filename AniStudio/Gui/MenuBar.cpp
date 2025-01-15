#include "MenuBar.hpp"
#include "../Events/Events.hpp"

namespace GUI {

static bool showViewListManager = false;

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
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("View List Manager", nullptr, showViewListManager)) {
                showViewListManager = !showViewListManager;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

bool ShouldRenderViewListManager() { return showViewListManager; }

} // namespace GUI