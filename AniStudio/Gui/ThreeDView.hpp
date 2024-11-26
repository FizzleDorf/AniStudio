#ifndef THREE_D_VIEW_HPP
#define THREE_D_VIEW_HPP

#include "ECS.h"
#include <GL/glew.h>
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <memory>

class ThreeDView {
public:
    // Constructor
    ThreeDView();

    // Main render function
    void Render();

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
    ECS::EntityManager &mgr = ECS::EntityManager::Ref(); // Using the EntityManager from ECS
    ECS::EntityID entity;                                // Entity for the 3D model
};

#endif // THREE_D_VIEW_HPP
