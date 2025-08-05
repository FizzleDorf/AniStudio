#pragma once

#include "BaseSystem.hpp"
#include "TransformComponent.hpp"
#include "MeshComponent.hpp"
#include "CameraComponent.hpp"
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ECS {

	class RenderSystem : public BaseSystem {
	public:
		RenderSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
			sysName = "RenderSystem";

			// This system requires Transform and Mesh components for renderable entities
			AddComponentSignature<TransformComponent>();
			AddComponentSignature<MeshComponent>();
		}

		void Start() override {
			InitializeShaders();

			// Setup basic 3D rendering state
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
		}

		void Update(const float deltaT) override {
			// Find active camera
			EntityID cameraEntity = GetActiveCameraEntity();
			if (cameraEntity == UINT32_MAX) {
				return; // No camera found
			}

			auto& camera = mgr.GetComponent<CameraComponent>(cameraEntity);

			// Clear the screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Use our shader program
			glUseProgram(shaderProgram);

			// Set view and projection matrices
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(camera.GetProjectionMatrix()));

			// Render all entities with Transform and Mesh components
			for (EntityID entity : entities) {
				if (mgr.HasComponent<TransformComponent>(entity) &&
					mgr.HasComponent<MeshComponent>(entity)) {

					auto& transform = mgr.GetComponent<TransformComponent>(entity);
					auto& mesh = mgr.GetComponent<MeshComponent>(entity);

					// Set model matrix
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform.GetTransformMatrix()));

					// Set color
					glUniform3f(colorLoc, mesh.color.x, mesh.color.y, mesh.color.z);

					// Draw the mesh
					mesh.Draw();
				}
			}

			glUseProgram(0);
		}

		void Destroy() override {
			if (shaderProgram != 0) {
				glDeleteProgram(shaderProgram);
				shaderProgram = 0;
			}
		}

		// Get the active camera entity (first camera found)
		EntityID GetActiveCameraEntity() const {
			for (EntityID entity = 0; entity < mgr.GetEntityCount(); ++entity) {
				if (mgr.IsEntityValid(entity) && mgr.HasComponent<CameraComponent>(entity)) {
					return entity;
				}
			}
			return UINT32_MAX; // No camera found
		}

		// Get camera component if available
		CameraComponent* GetActiveCamera() const {
			EntityID cameraEntity = GetActiveCameraEntity();
			if (cameraEntity != UINT32_MAX) {
				return &mgr.GetComponent<CameraComponent>(cameraEntity);
			}
			return nullptr;
		}

		// Public access to shader uniforms for ImGuizmo integration
		GLuint GetShaderProgram() const { return shaderProgram; }
		GLint GetModelLoc() const { return modelLoc; }
		GLint GetViewLoc() const { return viewLoc; }
		GLint GetProjLoc() const { return projLoc; }

	private:
		GLuint shaderProgram = 0;
		GLint modelLoc = -1;
		GLint viewLoc = -1;
		GLint projLoc = -1;
		GLint colorLoc = -1;

		void InitializeShaders() {
			// Vertex shader source
			const char* vertexShaderSource = R"(
                #version 330 core
                layout (location = 0) in vec3 aPos;
                layout (location = 1) in vec3 aNormal;
                layout (location = 2) in vec2 aTexCoord;

                out vec3 FragPos;
                out vec3 Normal;
                out vec2 TexCoord;

                uniform mat4 model;
                uniform mat4 view;
                uniform mat4 projection;

                void main() {
                    FragPos = vec3(model * vec4(aPos, 1.0));
                    Normal = mat3(transpose(inverse(model))) * aNormal;
                    TexCoord = aTexCoord;
                    
                    gl_Position = projection * view * vec4(FragPos, 1.0);
                }
            )";

			// Fragment shader source
			const char* fragmentShaderSource = R"(
                #version 330 core
                out vec4 FragColor;

                in vec3 FragPos;
                in vec3 Normal;
                in vec2 TexCoord;

                uniform vec3 objectColor;
                uniform vec3 lightPos = vec3(1.2, 1.0, 2.0);
                uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
                uniform vec3 viewPos = vec3(0.0, 0.0, 3.0);

                void main() {
                    // Ambient
                    float ambientStrength = 0.1;
                    vec3 ambient = ambientStrength * lightColor;
                    
                    // Diffuse
                    vec3 norm = normalize(Normal);
                    vec3 lightDir = normalize(lightPos - FragPos);
                    float diff = max(dot(norm, lightDir), 0.0);
                    vec3 diffuse = diff * lightColor;
                    
                    // Specular
                    float specularStrength = 0.5;
                    vec3 viewDir = normalize(viewPos - FragPos);
                    vec3 reflectDir = reflect(-lightDir, norm);
                    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
                    vec3 specular = specularStrength * spec * lightColor;
                    
                    vec3 result = (ambient + diffuse + specular) * objectColor;
                    FragColor = vec4(result, 1.0);
                }
            )";

			// Compile and link shaders
			GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
			GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

			shaderProgram = glCreateProgram();
			glAttachShader(shaderProgram, vertexShader);
			glAttachShader(shaderProgram, fragmentShader);
			glLinkProgram(shaderProgram);

			// Check for linking errors
			GLint success;
			char infoLog[512];
			glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
				std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			}

			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			// Get uniform locations
			modelLoc = glGetUniformLocation(shaderProgram, "model");
			viewLoc = glGetUniformLocation(shaderProgram, "view");
			projLoc = glGetUniformLocation(shaderProgram, "projection");
			colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
		}

		GLuint CompileShader(GLenum type, const char* source) {
			GLuint shader = glCreateShader(type);
			glShaderSource(shader, 1, &source, NULL);
			glCompileShader(shader);

			GLint success;
			char infoLog[512];
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shader, 512, NULL, infoLog);
				std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
			}

			return shader;
		}
	};

} // namespace ECS