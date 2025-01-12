// MeshView.hpp
#ifndef THREE_D_VIEW_HPP
#define THREE_D_VIEW_HPP

#include "Base/BaseView.hpp"
#include "pch.h"
#include <ImGuizmo.h>
#include <MeshSystem.hpp>
#include <RenderComponents/MeshComponent.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace GUI {
class MeshView : public BaseView {
public:
    // Constructor
    MeshView(ECS::EntityManager &entityMgr);

    // Main render function
    void Render() override;

    // Reset the view and gizmo
    void Reset();

    // Create an entity with a mesh component
    void CreateEntityWithMesh();

    // Update projection matrix on window resize
    void UpdateProjection(int width, int height);

    // Shader program creation
    GLuint CreateShaderProgram(const char *vertexShaderPath, const char *fragmentShaderPath);
    GLuint LoadShader(const char *shaderPath, GLenum shaderType);

private:
    float aspectRatio;

    // View and Projection matrices
    glm::mat4 cameraView;
    glm::mat4 cameraProjection;
    glm::mat4 objectMatrix;

    // Gizmo settings
    ImGuizmo::OPERATION currentGizmoOperation;
    ImGuizmo::MODE currentGizmoMode;

    // ECS related
    ECS::EntityID entity; // Entity for the 3D model
};
} // namespace GUI

#endif // THREE_D_VIEW_HPP