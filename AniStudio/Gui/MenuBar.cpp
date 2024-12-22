// MenuBar.cpp

#include "MenuBar.hpp"
#include "pch.h"
#include "ViewState.hpp"
#include "../Events/Events.hpp"

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
            if(ImGui::MenuItem("Exit")) {
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
            ImGui::MenuItem("Debug View", NULL, &viewState.showDebugView);
            ImGui::MenuItem("Diffusion View", NULL, &viewState.showDiffusionView);
            ImGui::MenuItem("Upscale View", NULL, &viewState.showDiffusionView);
            ImGui::MenuItem("Drawing Canvas", NULL, &viewState.showDrawingCanvas);
            ImGui::MenuItem("Settings", NULL, &viewState.showSettingsView);
            ImGui::MenuItem("MeshView", NULL, &viewState.showMeshView);
            ImGui::MenuItem("NodeGraph", NULL, &viewState.showNodeGraphView);
            ImGui::MenuItem("Sequencer", NULL, &viewState.showSequencerView);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
