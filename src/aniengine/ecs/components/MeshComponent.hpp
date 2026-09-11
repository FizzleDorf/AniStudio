#pragma once

#include "BaseComponent.hpp"
#include "OpenGLWrapper.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>

namespace ECS {

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
	};

	struct MeshComponent : public BaseComponent {
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		unsigned int VAO = 0;
		unsigned int VBO = 0;
		unsigned int EBO = 0;

		unsigned int textureID = 0;
		glm::vec3 color = glm::vec3(1.0f);

		std::string meshPath;
		std::string meshName;
		bool isLoaded = false;

		MeshComponent() = default;

		const char* GetCompName() const override { return "Mesh"; }
		const char* GetCompCategory() const override { return "3D"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
				{"title", "Mesh"},
				{"type", "object"},
				{"properties", {
					{"meshPath", {"type", "string", {"default", ""}}},
					{"meshName", {"type", "string", {"default", ""}}},
					{"color", {
						{"type", "object"},
						{"properties", {
							{"r", {"type", "number", {"default", 1.0f}, {"minimum", 0.0f}, {"maximum", 1.0f}}},
							{"g", {"type", "number", {"default", 1.0f}, {"minimum", 0.0f}, {"maximum", 1.0f}}},
							{"b", {"type", "number", {"default", 1.0f}, {"minimum", 0.0f}, {"maximum", 1.0f}}}
						}}
					}}
				}},
				{"inputs", {
					{{"name", "transform"}, {"type", "mat4"}}
				}},
				{"outputs", {
					{{"name", "rendered_mesh"}, {"type", "mesh"}}
				}}
			};
			return j;
		}

		~MeshComponent() {
			CleanupGL();
		}

		void CleanupGL() {
			if (VAO != 0) {
				glDeleteVertexArrays(1, &VAO);
				VAO = 0;
			}
			if (VBO != 0) {
				glDeleteBuffers(1, &VBO);
				VBO = 0;
			}
			if (EBO != 0) {
				glDeleteBuffers(1, &EBO);
				EBO = 0;
			}
		}

		void SetupMesh() {
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

			glBindVertexArray(0);
			isLoaded = true;
		}

		void Draw() const {
			if (!isLoaded || VAO == 0) return;

			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"meshPath", &meshPath},
				{"meshName", &meshName},
				{"color_r", &color.r},
				{"color_g", &color.g},
				{"color_b", &color.b}
			};
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j[GetCompName()] = {
				{"meshPath", meshPath},
				{"meshName", meshName},
				{"color", { color.r, color.g, color.b }},
				{"isLoaded", isLoaded}
			};
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			const char* key = GetCompName();
			nlohmann::json componentData;
			if (j.contains(key))
				componentData = j.at(key);
			else
				componentData = j;

			if (componentData.contains("meshPath")) meshPath = componentData["meshPath"];
			if (componentData.contains("meshName")) meshName = componentData["meshName"];
			if (componentData.contains("color") && componentData["color"].is_array() && componentData["color"].size() >= 3) {
				color = glm::vec3(componentData["color"][0], componentData["color"][1], componentData["color"][2]);
			}
			if (componentData.contains("isLoaded")) isLoaded = componentData["isLoaded"];
		}
	};

} // namespace ECS