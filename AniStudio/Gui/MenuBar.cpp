// MenuBar.cpp
#include "imgui.h"
#include "Engine/Engine.hpp"
#include "GLFW/glfw3.h"
#include "ViewState.hpp"
#include "MenuBar.hpp"


void ShowMenuBar(GLFWwindow* window) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {

            };
            ImGui::MenuItem("Save", "Ctrl+S");
            if(ImGui::MenuItem("Exit", "Ctrl+Q")) {
                ANI::Core.Quit();
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
            //ImGui::MenuItem("Diffusion View", NULL, &viewState.showDynamicDiffusionView);
            ImGui::MenuItem("Diffusion View", NULL, &viewState.showDiffusionView);
            ImGui::MenuItem("Drawing Canvas", NULL, &viewState.showDrawingCanvas);
            ImGui::MenuItem("Settings", NULL, &viewState.showSettingsView);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
