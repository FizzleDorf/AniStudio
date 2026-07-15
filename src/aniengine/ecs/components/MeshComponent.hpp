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

		// OpenGL objects
		unsigned int VAO = 0;
		unsigned int VBO = 0;
		unsigned int EBO = 0;

		// Material properties
		unsigned int textureID = 0;
		glm::vec3 color = glm::vec3(1.0f);

		// Mesh info
		std::string meshPath;
		std::string meshName;
		bool isLoaded = false;

		MeshComponent() {
			compName = "Mesh";
			compCategory = "3D";

			schema = {
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
			// Generate and bind VAO
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);

			// Load vertex data
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

			// Load index data
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

			// Set vertex attribute pointers
			// Position attribute
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

			// Normal attribute
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

			// Texture coordinate attribute
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

			glBindVertexArray(0);
			isLoaded = true;
		}

		void Draw() const {
			if (!isLoaded || VAO == 0) return;

			glBindVertexArray(VAO);
			// Explicit cast to avoid C4267 warning (size_t Å® GLsizei)
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

		// Get property map for UI rendering
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"meshPath", &meshPath},
				{"meshName", &meshName},
				{"color_r", &color.r},
				{"color_g", &color.g},
				{"color_b", &color.b}
			};
		}

		// Serialization
		nlohmann::json Serialize() const override {
			auto j = BaseComponent::Serialize();
			j["meshPath"] = meshPath;
			j["meshName"] = meshName;
			j["color"] = { color.r, color.g, color.b };
			j["isLoaded"] = isLoaded;
			return j;
		}

		// Deserialization
		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			if (j.contains("meshPath")) {
				meshPath = j["meshPath"];
			}
			if (j.contains("meshName")) {
				meshName = j["meshName"];
			}
			if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3) {
				color = glm::vec3(j["color"][0], j["color"][1], j["color"][2]);
			}
			if (j.contains("isLoaded")) {
				isLoaded = j["isLoaded"];
			}
		}
	};

} // namespace ECS