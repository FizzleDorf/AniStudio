#ifndef THREE_D_VIEW_HPP
#define THREE_D_VIEW_HPP

#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

class ThreeDView {
public:
    ThreeDView();
    void Render();
    void Reset();

private:
    glm::mat4 cameraView;
    glm::mat4 cameraProjection;
    glm::mat4 objectMatrix;
    ImGuizmo::OPERATION currentGizmoOperation;
    ImGuizmo::MODE currentGizmoMode;
};

#endif // THREE_D_VIEW_HPP
