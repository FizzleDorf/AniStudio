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

		mutable glm::mat4 transformMatrix = glm::mat4(1.0f);
		mutable bool isDirty = true;

		TransformComponent() = default;

		const char* GetCompName() const override { return "Transform"; }
		const char* GetCompCategory() const override { return "3D"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
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
			return j;
		}

		TransformComponent(const TransformComponent& other) : BaseComponent(other) {
			position = other.position;
			rotation = other.rotation;
			scale = other.scale;
			transformMatrix = other.transformMatrix;
			isDirty = true;
		}

		TransformComponent& operator=(const TransformComponent& other) {
			if (this != &other) {
				position = other.position;
				rotation = other.rotation;
				scale = other.scale;
				transformMatrix = other.transformMatrix;
				isDirty = true;
			}
			return *this;
		}

		const glm::mat4& GetTransformMatrix() const {
			if (isDirty) {
				UpdateTransformMatrix();
				isDirty = false;
			}
			return transformMatrix;
		}

		void SetPosition(const glm::vec3& pos) {
			position = pos;
			isDirty = true;
		}

		void SetRotation(const glm::vec3& rot) {
			rotation = rot;
			isDirty = true;
		}

		void SetScale(const glm::vec3& scl) {
			scale = scl;
			isDirty = true;
		}

		void SetFromMatrix(const glm::mat4& matrix) {
			DecomposeMatrix(matrix);
			isDirty = true;
		}

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

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j[GetCompName()] = {
				{"position", { position.x, position.y, position.z }},
				{"rotation", { rotation.x, rotation.y, rotation.z }},
				{"scale", { scale.x, scale.y, scale.z }}
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

			if (componentData.contains("position") && componentData["position"].is_array() && componentData["position"].size() >= 3) {
				position = glm::vec3(componentData["position"][0], componentData["position"][1], componentData["position"][2]);
			}

			if (componentData.contains("rotation") && componentData["rotation"].is_array() && componentData["rotation"].size() >= 3) {
				rotation = glm::vec3(componentData["rotation"][0], componentData["rotation"][1], componentData["rotation"][2]);
			}

			if (componentData.contains("scale") && componentData["scale"].is_array() && componentData["scale"].size() >= 3) {
				scale = glm::vec3(componentData["scale"][0], componentData["scale"][1], componentData["scale"][2]);
			}

			isDirty = true;
		}

	private:
		void UpdateTransformMatrix() const {
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 rotationMatrix = rotationZ * rotationY * rotationX;
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

			transformMatrix = translation * rotationMatrix * scaleMatrix;
		}

		void DecomposeMatrix(const glm::mat4& matrix) {
			position = glm::vec3(matrix[3]);

			scale.x = glm::length(glm::vec3(matrix[0]));
			scale.y = glm::length(glm::vec3(matrix[1]));
			scale.z = glm::length(glm::vec3(matrix[2]));

			glm::mat3 rotMatrix = glm::mat3(
				glm::vec3(matrix[0]) / scale.x,
				glm::vec3(matrix[1]) / scale.y,
				glm::vec3(matrix[2]) / scale.z
			);

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