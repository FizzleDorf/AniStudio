#include "ThreeDView.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>  // <-- Add this line
#include <ImGuizmo.h>
#include <imgui.h>
#include <iostream>


ThreeDView::ThreeDView()
    : currentGizmoOperation(ImGuizmo::TRANSLATE), currentGizmoMode(ImGuizmo::WORLD),
      cameraView(glm::lookAt(glm::vec3(10.f, 10.f, 10.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f))),
      cameraProjection(glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f)), objectMatrix(glm::mat4(1.0f)) {}

void ThreeDView::Reset() {
    currentGizmoOperation = ImGuizmo::TRANSLATE;
    currentGizmoMode = ImGuizmo::WORLD;
    objectMatrix = glm::mat4(1.0f);
}

void ThreeDView::Render() {
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
    ImGui::Begin("3D View");

    // Adjust ImGuizmo operation type
    if (ImGui::IsKeyPressed(ImGuiKey_W))
        currentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_E))
        currentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R))
        currentGizmoOperation = ImGuizmo::SCALE;

    // Camera controls
    ImGui::Text("Camera Projection");
    if (ImGui::RadioButton("Perspective", currentGizmoMode == ImGuizmo::WORLD))
        currentGizmoMode = ImGuizmo::WORLD;
    ImGui::SameLine();
    if (ImGui::RadioButton("Orthographic", currentGizmoMode == ImGuizmo::LOCAL))
        currentGizmoMode = ImGuizmo::LOCAL;

    ImGuizmo::SetOrthographic(currentGizmoMode == ImGuizmo::LOCAL);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowSize().x,
                      ImGui::GetWindowSize().y);

    // Display the gizmo for manipulating the object matrix
    ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), currentGizmoOperation,
                         currentGizmoMode, glm::value_ptr(objectMatrix));

    // Display the matrix
    ImGui::Separator();
    ImGui::Text("Object Matrix:");
    for (int i = 0; i < 4; ++i) {
        ImGui::Text("%.2f %.2f %.2f %.2f", objectMatrix[i][0], objectMatrix[i][1], objectMatrix[i][2],
                    objectMatrix[i][3]);
    }

    ImGui::End();
}
