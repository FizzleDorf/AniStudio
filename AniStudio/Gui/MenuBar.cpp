// MenuBar.cpp

#include "MenuBar.hpp"
#include "pch.h"
#include "../Events/Events.hpp"


void ShowMenuBar(GLFWwindow* window) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {

            };
            ImGui::MenuItem("Save", "Ctrl+S");
            if(ImGui::MenuItem("Exit", "Ctrl+Q")) {
                ANI::Event event;
                event.type = ANI::EventType::QuitRequest;
                ANI::Events::Ref().QueueEvent(event);
            };
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo", "Ctrl+Z");
            ImGui::MenuItem("Redo", "Ctrl+Y");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
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
