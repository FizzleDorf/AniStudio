#pragma once

#include "BaseComponent.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ECS {

	struct CameraComponent : public BaseComponent {
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
		glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

		float yaw = -90.0f;
		float pitch = 0.0f;

		float fov = 45.0f;
		float aspectRatio = 16.0f / 9.0f;
		float nearPlane = 0.1f;
		float farPlane = 100.0f;

		float movementSpeed = 2.5f;
		float mouseSensitivity = 0.1f;
		float zoom = 45.0f;

		mutable glm::mat4 viewMatrix = glm::mat4(1.0f);
		mutable glm::mat4 projectionMatrix = glm::mat4(1.0f);
		mutable bool viewDirty = true;
		mutable bool projectionDirty = true;

		CameraComponent() {
			UpdateCameraVectors();
		}

		const char* GetCompName() const override { return "Camera"; }
		const char* GetCompCategory() const override { return "3D"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
				{"title", "Camera"},
				{"type", "object"},
				{"properties", {
					{"position", {
						{"type", "object"},
						{"properties", {
							{"x", {"type", "number", {"default", 0.0f}}},
							{"y", {"type", "number", {"default", 0.0f}}},
							{"z", {"type", "number", {"default", 3.0f}}}
						}}
					}},
					{"fov", {"type", "number", {"default", 45.0f}, {"minimum", 1.0f}, {"maximum", 120.0f}}},
					{"nearPlane", {"type", "number", {"default", 0.1f}, {"minimum", 0.01f}}},
					{"farPlane", {"type", "number", {"default", 100.0f}, {"minimum", 1.0f}}},
					{"movementSpeed", {"type", "number", {"default", 2.5f}, {"minimum", 0.1f}}},
					{"mouseSensitivity", {"type", "number", {"default", 0.1f}, {"minimum", 0.01f}}}
				}},
				{"outputs", {
					{{"name", "view_matrix"}, {"type", "mat4"}},
					{{"name", "projection_matrix"}, {"type", "mat4"}}
				}}
			};
			return j;
		}

		CameraComponent(const CameraComponent& other) : BaseComponent(other) {
			position = other.position;
			front = other.front;
			up = other.up;
			right = other.right;
			worldUp = other.worldUp;
			yaw = other.yaw;
			pitch = other.pitch;
			fov = other.fov;
			aspectRatio = other.aspectRatio;
			nearPlane = other.nearPlane;
			farPlane = other.farPlane;
			movementSpeed = other.movementSpeed;
			mouseSensitivity = other.mouseSensitivity;
			zoom = other.zoom;
			viewMatrix = other.viewMatrix;
			projectionMatrix = other.projectionMatrix;
			viewDirty = true;
			projectionDirty = true;
		}

		CameraComponent& operator=(const CameraComponent& other) {
			if (this != &other) {
				position = other.position;
				front = other.front;
				up = other.up;
				right = other.right;
				worldUp = other.worldUp;
				yaw = other.yaw;
				pitch = other.pitch;
				fov = other.fov;
				aspectRatio = other.aspectRatio;
				nearPlane = other.nearPlane;
				farPlane = other.farPlane;
				movementSpeed = other.movementSpeed;
				mouseSensitivity = other.mouseSensitivity;
				zoom = other.zoom;
				viewMatrix = other.viewMatrix;
				projectionMatrix = other.projectionMatrix;
				viewDirty = true;
				projectionDirty = true;
			}
			return *this;
		}

		const glm::mat4& GetViewMatrix() const {
			if (viewDirty) {
				viewMatrix = glm::lookAt(position, position + front, up);
				viewDirty = false;
			}
			return viewMatrix;
		}

		const glm::mat4& GetProjectionMatrix() const {
			if (projectionDirty) {
				projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
				projectionDirty = false;
			}
			return projectionMatrix;
		}

		void SetAspectRatio(float ratio) {
			aspectRatio = ratio;
			projectionDirty = true;
		}

		void SetPosition(const glm::vec3& pos) {
			position = pos;
			viewDirty = true;
		}

		void SetFOV(float newFov) {
			fov = glm::clamp(newFov, 1.0f, 120.0f);
			projectionDirty = true;
		}

		void ProcessKeyboard(int direction, float deltaTime) {
			float velocity = movementSpeed * deltaTime;

			if (direction == 0)
				position += front * velocity;
			if (direction == 1)
				position -= front * velocity;
			if (direction == 2)
				position -= right * velocity;
			if (direction == 3)
				position += right * velocity;
			if (direction == 4)
				position += up * velocity;
			if (direction == 5)
				position -= up * velocity;

			viewDirty = true;
		}

		void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true) {
			xOffset *= mouseSensitivity;
			yOffset *= mouseSensitivity;

			yaw += xOffset;
			pitch += yOffset;

			if (constrainPitch) {
				if (pitch > 89.0f)
					pitch = 89.0f;
				if (pitch < -89.0f)
					pitch = -89.0f;
			}

			UpdateCameraVectors();
		}

		void ProcessMouseScroll(float yOffset) {
			zoom -= yOffset;
			if (zoom < 1.0f)
				zoom = 1.0f;
			if (zoom > 45.0f)
				zoom = 45.0f;
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"position_x", &position.x},
				{"position_y", &position.y},
				{"position_z", &position.z},
				{"fov", &fov},
				{"nearPlane", &nearPlane},
				{"farPlane", &farPlane},
				{"movementSpeed", &movementSpeed},
				{"mouseSensitivity", &mouseSensitivity}
			};
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j[GetCompName()] = {
				{"position", { position.x, position.y, position.z }},
				{"yaw", yaw},
				{"pitch", pitch},
				{"fov", fov},
				{"aspectRatio", aspectRatio},
				{"nearPlane", nearPlane},
				{"farPlane", farPlane},
				{"movementSpeed", movementSpeed},
				{"mouseSensitivity", mouseSensitivity}
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
			if (componentData.contains("yaw")) yaw = componentData["yaw"];
			if (componentData.contains("pitch")) pitch = componentData["pitch"];
			if (componentData.contains("fov")) fov = componentData["fov"];
			if (componentData.contains("aspectRatio")) aspectRatio = componentData["aspectRatio"];
			if (componentData.contains("nearPlane")) nearPlane = componentData["nearPlane"];
			if (componentData.contains("farPlane")) farPlane = componentData["farPlane"];
			if (componentData.contains("movementSpeed")) movementSpeed = componentData["movementSpeed"];
			if (componentData.contains("mouseSensitivity")) mouseSensitivity = componentData["mouseSensitivity"];

			UpdateCameraVectors();
			viewDirty = true;
			projectionDirty = true;
		}

	private:
		void UpdateCameraVectors() {
			glm::vec3 frontVec;
			frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			frontVec.y = sin(glm::radians(pitch));
			frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			front = glm::normalize(frontVec);

			right = glm::normalize(glm::cross(front, worldUp));
			up = glm::normalize(glm::cross(right, front));

			viewDirty = true;
		}
	};

} // namespace ECS