#pragma once

#include "BaseComponent.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ECS {

	struct TransformComponent : public BaseComponent {
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);

		// Cached transform matrix
		mutable glm::mat4 transformMatrix = glm::mat4(1.0f);
		mutable bool isDirty = true;

		TransformComponent() {
			compName = "Transform";
			compCategory = "3D";

			// Define schema for node inputs/outputs and UI
			schema = {
				{"title", "Transform"},
				{"type", "object"},
				{"properties", {
					{"position", {
						{"type", "object"},
						{"properties", {
							{"x", {"type", "number", {"default", 0.0f}}},
							{"y", {"type", "number", {"default", 0.0f}}},
							{"z", {"type", "number", {"default", 0.0f}}}
						}}
					}},
					{"rotation", {
						{"type", "object"},
						{"properties", {
							{"x", {"type", "number", {"default", 0.0f}}},
							{"y", {"type", "number", {"default", 0.0f}}},
							{"z", {"type", "number", {"default", 0.0f}}}
						}}
					}},
					{"scale", {
						{"type", "object"},
						{"properties", {
							{"x", {"type", "number", {"default", 1.0f}}},
							{"y", {"type", "number", {"default", 1.0f}}},
							{"z", {"type", "number", {"default", 1.0f}}}
						}}
					}}
				}},
				{"outputs", {
					{{"name", "transform_matrix"}, {"type", "mat4"}}
				}}
			};
		}

		// Get the transform matrix (cached and only recalculated when dirty)
		const glm::mat4& GetTransformMatrix() const {
			if (isDirty) {
				UpdateTransformMatrix();
				isDirty = false;
			}
			return transformMatrix;
		}

		// Set position and mark dirty
		void SetPosition(const glm::vec3& pos) {
			position = pos;
			isDirty = true;
		}

		// Set rotation (Euler angles in radians) and mark dirty
		void SetRotation(const glm::vec3& rot) {
			rotation = rot;
			isDirty = true;
		}

		// Set scale and mark dirty
		void SetScale(const glm::vec3& scl) {
			scale = scl;
			isDirty = true;
		}

		// Set transform from matrix (decompose and mark dirty)
		void SetFromMatrix(const glm::mat4& matrix) {
			DecomposeMatrix(matrix);
			isDirty = true;
		}

		// Get property map for UI rendering
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"position_x", &position.x},
				{"position_y", &position.y},
				{"position_z", &position.z},
				{"rotation_x", &rotation.x},
				{"rotation_y", &rotation.y},
				{"rotation_z", &rotation.z},
				{"scale_x", &scale.x},
				{"scale_y", &scale.y},
				{"scale_z", &scale.z}
			};
		}

		// Serialization
		nlohmann::json Serialize() const override {
			auto j = BaseComponent::Serialize();
			j["position"] = { position.x, position.y, position.z };
			j["rotation"] = { rotation.x, rotation.y, rotation.z };
			j["scale"] = { scale.x, scale.y, scale.z };
			return j;
		}

		// Deserialization
		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
				position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
			}

			if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() >= 3) {
				rotation = glm::vec3(j["rotation"][0], j["rotation"][1], j["rotation"][2]);
			}

			if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() >= 3) {
				scale = glm::vec3(j["scale"][0], j["scale"][1], j["scale"][2]);
			}

			isDirty = true;
		}

	private:
		void UpdateTransformMatrix() const {
			// Create transformation matrix: T * R * S
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 rotationMatrix = rotationZ * rotationY * rotationX;
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

			transformMatrix = translation * rotationMatrix * scaleMatrix;
		}

		void DecomposeMatrix(const glm::mat4& matrix) {
			// Extract translation
			position = glm::vec3(matrix[3]);

			// Extract scale
			scale.x = glm::length(glm::vec3(matrix[0]));
			scale.y = glm::length(glm::vec3(matrix[1]));
			scale.z = glm::length(glm::vec3(matrix[2]));

			// Remove scaling from the matrix
			glm::mat3 rotMatrix = glm::mat3(
				glm::vec3(matrix[0]) / scale.x,
				glm::vec3(matrix[1]) / scale.y,
				glm::vec3(matrix[2]) / scale.z
			);

			// Extract rotation (convert to Euler angles)
			rotation.y = asin(-rotMatrix[0][2]);
			if (cos(rotation.y) != 0) {
				rotation.x = atan2(rotMatrix[1][2], rotMatrix[2][2]);
				rotation.z = atan2(rotMatrix[0][1], rotMatrix[0][0]);
			}
			else {
				rotation.x = atan2(-rotMatrix[2][1], rotMatrix[1][1]);
				rotation.z = 0;
			}
		}
	};

} // namespace ECS