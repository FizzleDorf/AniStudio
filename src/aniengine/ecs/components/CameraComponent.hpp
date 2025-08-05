#pragma once

#include "BaseComponent.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ECS {

	struct CameraComponent : public BaseComponent {
		// Camera parameters
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
		glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

		// Euler angles
		float yaw = -90.0f;
		float pitch = 0.0f;

		// Projection parameters
		float fov = 45.0f;
		float aspectRatio = 16.0f / 9.0f;
		float nearPlane = 0.1f;
		float farPlane = 100.0f;

		// Camera options
		float movementSpeed = 2.5f;
		float mouseSensitivity = 0.1f;
		float zoom = 45.0f;

		// Cached matrices
		mutable glm::mat4 viewMatrix = glm::mat4(1.0f);
		mutable glm::mat4 projectionMatrix = glm::mat4(1.0f);
		mutable bool viewDirty = true;
		mutable bool projectionDirty = true;

		CameraComponent() {
			compName = "Camera";
			compCategory = "3D";

			schema = {
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

			UpdateCameraVectors();
		}

		// Get view matrix
		const glm::mat4& GetViewMatrix() const {
			if (viewDirty) {
				viewMatrix = glm::lookAt(position, position + front, up);
				viewDirty = false;
			}
			return viewMatrix;
		}

		// Get projection matrix
		const glm::mat4& GetProjectionMatrix() const {
			if (projectionDirty) {
				projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
				projectionDirty = false;
			}
			return projectionMatrix;
		}

		// Set aspect ratio and mark projection dirty
		void SetAspectRatio(float ratio) {
			aspectRatio = ratio;
			projectionDirty = true;
		}

		// Set position and mark view dirty
		void SetPosition(const glm::vec3& pos) {
			position = pos;
			viewDirty = true;
		}

		// Set field of view and mark projection dirty
		void SetFOV(float newFov) {
			fov = glm::clamp(newFov, 1.0f, 120.0f);
			projectionDirty = true;
		}

		// Process keyboard input
		void ProcessKeyboard(int direction, float deltaTime) {
			float velocity = movementSpeed * deltaTime;

			if (direction == 0) // FORWARD
				position += front * velocity;
			if (direction == 1) // BACKWARD
				position -= front * velocity;
			if (direction == 2) // LEFT
				position -= right * velocity;
			if (direction == 3) // RIGHT
				position += right * velocity;
			if (direction == 4) // UP
				position += up * velocity;
			if (direction == 5) // DOWN
				position -= up * velocity;

			viewDirty = true;
		}

		// Process mouse movement
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

		// Process mouse scroll
		void ProcessMouseScroll(float yOffset) {
			zoom -= yOffset;
			if (zoom < 1.0f)
				zoom = 1.0f;
			if (zoom > 45.0f)
				zoom = 45.0f;
		}

		// Get property map for UI rendering
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

		// Serialization
		nlohmann::json Serialize() const override {
			auto j = BaseComponent::Serialize();
			j["position"] = { position.x, position.y, position.z };
			j["yaw"] = yaw;
			j["pitch"] = pitch;
			j["fov"] = fov;
			j["aspectRatio"] = aspectRatio;
			j["nearPlane"] = nearPlane;
			j["farPlane"] = farPlane;
			j["movementSpeed"] = movementSpeed;
			j["mouseSensitivity"] = mouseSensitivity;
			return j;
		}

		// Deserialization
		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
				position = glm::vec3(j["position"][0], j["position"][1], j["position"][2]);
			}
			if (j.contains("yaw")) yaw = j["yaw"];
			if (j.contains("pitch")) pitch = j["pitch"];
			if (j.contains("fov")) fov = j["fov"];
			if (j.contains("aspectRatio")) aspectRatio = j["aspectRatio"];
			if (j.contains("nearPlane")) nearPlane = j["nearPlane"];
			if (j.contains("farPlane")) farPlane = j["farPlane"];
			if (j.contains("movementSpeed")) movementSpeed = j["movementSpeed"];
			if (j.contains("mouseSensitivity")) mouseSensitivity = j["mouseSensitivity"];

			UpdateCameraVectors();
			viewDirty = true;
			projectionDirty = true;
		}

	private:
		void UpdateCameraVectors() {
			// Calculate the new front vector
			glm::vec3 frontVec;
			frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			frontVec.y = sin(glm::radians(pitch));
			frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			front = glm::normalize(frontVec);

			// Re-calculate the right and up vector
			right = glm::normalize(glm::cross(front, worldUp));
			up = glm::normalize(glm::cross(right, front));

			viewDirty = true;
		}
	};

} // namespace ECS