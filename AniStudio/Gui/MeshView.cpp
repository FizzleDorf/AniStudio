#include "MeshView.hpp"
#include <GL/glew.h>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

using namespace ECS;

MeshView::MeshView()
    : currentGizmoOperation(ImGuizmo::TRANSLATE), currentGizmoMode(ImGuizmo::WORLD),
      cameraView(glm::lookAt(glm::vec3(10.f, 10.f, 10.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f))),
      cameraProjection(glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f)), objectMatrix(glm::mat4(1.0f)),
      aspectRatio(1.0f) {
    UpdateProjection(1280, 720); // Default aspect ratio
    // CreateEntityWithMesh();      // Create entity with mesh
}

void MeshView::Reset() {
    currentGizmoOperation = ImGuizmo::TRANSLATE;
    currentGizmoMode = ImGuizmo::WORLD;
    objectMatrix = glm::mat4(1.0f);
}

void MeshView::UpdateProjection(int width, int height) {
    if (height == 0) {
        std::cerr << "Invalid height for projection matrix update." << std::endl;
        return;
    }

    float newAspectRatio = static_cast<float>(width) / height;
    if (newAspectRatio != aspectRatio) {
        aspectRatio = newAspectRatio;
        cameraProjection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    }
}

void MeshView::CreateEntityWithMesh() {
    entity = mgr.AddNewEntity();

    mgr.AddComponent<MeshComponent>(entity);
    mgr.GetComponent<MeshComponent>(entity).vertices = {glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f),
                                                        glm::vec3(0.0f, 1.0f, 0.0f)};
    mgr.GetComponent<MeshComponent>(entity).normals = {glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                                                       glm::vec3(0.0f, 0.0f, 1.0f)};
    mgr.GetComponent<MeshComponent>(entity).uvs = {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.5f, 1.0f)};
    mgr.GetComponent<MeshComponent>(entity).indices = {0, 1, 2};
}

GLuint MeshView::LoadShader(const char *shaderPath, GLenum shaderType) {
    std::ifstream shaderFile(shaderPath);
    if (!shaderFile.is_open()) {
        std::cerr << "Error opening shader file: " << shaderPath << std::endl;
        return 0;
    }

    std::string shaderCode((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    const char *shaderSource = shaderCode.c_str();

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Error compiling shader: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint MeshView::CreateShaderProgram(const char *vertexShaderPath, const char *fragmentShaderPath) {
    GLuint vertexShader = LoadShader(vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = LoadShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

    if (vertexShader == 0 || fragmentShader == 0) {
        std::cerr << "Shader loading failed!" << std::endl;
        return 0;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Error linking shader program: " << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void MeshView::Render() {
    ImVec2 windowSize = ImGui::GetWindowSize();
    UpdateProjection(static_cast<int>(windowSize.x), static_cast<int>(windowSize.y));

    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
    ImGui::Begin("3D View");

    if (ImGui::IsKeyPressed(ImGuiKey_W))
        currentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_E))
        currentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R))
        currentGizmoOperation = ImGuizmo::SCALE;

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowSize().x,
                      ImGui::GetWindowSize().y);
    ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), currentGizmoOperation,
                         currentGizmoMode, glm::value_ptr(objectMatrix));

    GLuint shaderProgram = CreateShaderProgram("../shaders/basic.vert", "../shaders/basic.frag");

    /*if (shaderProgram != 0) {
        ECS::MeshComponent &mesh = mgr.GetComponent<ECS::MeshComponent>(entity);

        static GLuint VAO = 0, VBO = 0, EBO = 0;
        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER,
                         mesh.vertices.size() * sizeof(glm::vec3) + mesh.normals.size() * sizeof(glm::vec3) +
                             mesh.uvs.size() * sizeof(glm::vec2),
                         nullptr, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertices.size() * sizeof(glm::vec3), mesh.vertices.data());
            glBufferSubData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3),
                            mesh.normals.size() * sizeof(glm::vec3), mesh.normals.data());
            glBufferSubData(GL_ARRAY_BUFFER,
                            mesh.vertices.size() * sizeof(glm::vec3) + mesh.normals.size() * sizeof(glm::vec3),
                            mesh.uvs.size() * sizeof(glm::vec2), mesh.uvs.data());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(),
                         GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                                  (void *)(mesh.vertices.size() * sizeof(glm::vec3)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(
                2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2),
                (void *)(mesh.vertices.size() * sizeof(glm::vec3) + mesh.normals.size() * sizeof(glm::vec3)));
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
        }

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(cameraView));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                           glm::value_ptr(cameraProjection));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }*/

    ImGui::End();
}
