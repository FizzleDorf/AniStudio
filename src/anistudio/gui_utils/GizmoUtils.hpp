#pragma once

#include <ImGuizmo.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Utils {

	class GizmoUtils {
	public:
		enum class Operation {
			TRANSLATE = ImGuizmo::TRANSLATE,
			ROTATE = ImGuizmo::ROTATE,
			SCALE = ImGuizmo::SCALE,
			UNIVERSAL = ImGuizmo::UNIVERSAL
		};

		enum class Mode {
			LOCAL = ImGuizmo::LOCAL,
			WORLD = ImGuizmo::WORLD
		};

		struct GizmoState {
			Operation operation = Operation::TRANSLATE;
			Mode mode = Mode::WORLD;
			bool useSnap = false;
			float snapValues[3] = { 1.0f, 1.0f, 1.0f };
		};

		static void Initialize() {
			ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
		}

		static bool RenderGizmo(const glm::mat4& viewMatrix,
			const glm::mat4& projMatrix,
			glm::mat4& modelMatrix,
			const GizmoState& state,
			const ImVec2& viewportPos,
			const ImVec2& viewportSize) {

			// Set ImGuizmo rect to match viewport
			ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

			// Render the gizmo
			return ImGuizmo::Manipulate(
				glm::value_ptr(viewMatrix),
				glm::value_ptr(projMatrix),
				static_cast<ImGuizmo::OPERATION>(state.operation),
				static_cast<ImGuizmo::MODE>(state.mode),
				glm::value_ptr(modelMatrix),
				nullptr, // deltaMatrix
				state.useSnap ? state.snapValues : nullptr
			);
		}

		static bool IsUsing() {
			return ImGuizmo::IsUsing();
		}


		static bool IsOver() {
			return ImGuizmo::IsOver();
		}

		static bool HandleHotkeys(GizmoState& state) {
			bool changed = false;

			if (ImGui::IsKeyPressed(ImGuiKey_W)) {
				state.operation = Operation::TRANSLATE;
				changed = true;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_E)) {
				state.operation = Operation::ROTATE;
				changed = true;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
				state.operation = Operation::SCALE;
				changed = true;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_T)) {
				state.operation = Operation::UNIVERSAL;
				changed = true;
			}

			// Toggle between local and world mode
			if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
				state.mode = (state.mode == Mode::LOCAL) ? Mode::WORLD : Mode::LOCAL;
				changed = true;
			}

			return changed;
		}

		static void RenderSettingsPanel(GizmoState& state) {
			if (ImGui::Begin("Gizmo Settings")) {
				// Operation selection
				ImGui::Text("Operation:");

				bool isTranslate = (state.operation == Operation::TRANSLATE);
				bool isRotate = (state.operation == Operation::ROTATE);
				bool isScale = (state.operation == Operation::SCALE);
				bool isUniversal = (state.operation == Operation::UNIVERSAL);

				if (ImGui::RadioButton("Translate (W)", isTranslate)) {
					state.operation = Operation::TRANSLATE;
				}
				if (ImGui::RadioButton("Rotate (E)", isRotate)) {
					state.operation = Operation::ROTATE;
				}
				if (ImGui::RadioButton("Scale (R)", isScale)) {
					state.operation = Operation::SCALE;
				}
				if (ImGui::RadioButton("Universal (T)", isUniversal)) {
					state.operation = Operation::UNIVERSAL;
				}

				ImGui::Separator();

				// Mode selection
				ImGui::Text("Mode:");

				bool isLocal = (state.mode == Mode::LOCAL);
				bool isWorld = (state.mode == Mode::WORLD);

				if (ImGui::RadioButton("Local", isLocal)) {
					state.mode = Mode::LOCAL;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("World (Space)", isWorld)) {
					state.mode = Mode::WORLD;
				}

				ImGui::Separator();

				// Snapping settings
				ImGui::Checkbox("Use Snapping", &state.useSnap);
				if (state.useSnap) {
					ImGui::Text("Snap Values:");

					if (state.operation == Operation::TRANSLATE) {
						ImGui::DragFloat3("Translation Snap", state.snapValues, 0.1f, 0.1f, 10.0f);
					}
					else if (state.operation == Operation::ROTATE) {
						ImGui::DragFloat("Rotation Snap (degrees)", &state.snapValues[0], 1.0f, 1.0f, 90.0f);
					}
					else if (state.operation == Operation::SCALE) {
						ImGui::DragFloat("Scale Snap", &state.snapValues[0], 0.01f, 0.01f, 1.0f);
					}
				}

				ImGui::Separator();

				// Status display
				ImGui::Text("Status:");
				const char* usingText = IsUsing() ? "Yes" : "No";
				const char* overText = IsOver() ? "Yes" : "No";
				ImGui::Text("Using Gizmo: %s", usingText);
				ImGui::Text("Over Gizmo: %s", overText);
			}
			ImGui::End();
		}

		static void DecomposeMatrix(const glm::mat4& matrix,
			glm::vec3& position,
			glm::vec3& rotation,
			glm::vec3& scale) {
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

		static glm::mat4 ComposeMatrix(const glm::vec3& position,
			const glm::vec3& rotation,
			const glm::vec3& scale) {
			// Create transformation matrix: T * R * S
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 rotationMatrix = rotationZ * rotationY * rotationX;
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

			return translation * rotationMatrix * scaleMatrix;
		}

		static void RenderMenuBar(GizmoState& state) {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("Gizmo")) {
					bool isTranslate = (state.operation == Operation::TRANSLATE);
					bool isRotate = (state.operation == Operation::ROTATE);
					bool isScale = (state.operation == Operation::SCALE);
					bool isUniversal = (state.operation == Operation::UNIVERSAL);
					bool isLocal = (state.mode == Mode::LOCAL);

					if (ImGui::MenuItem("Translate", "W", isTranslate)) {
						state.operation = Operation::TRANSLATE;
					}
					if (ImGui::MenuItem("Rotate", "E", isRotate)) {
						state.operation = Operation::ROTATE;
					}
					if (ImGui::MenuItem("Scale", "R", isScale)) {
						state.operation = Operation::SCALE;
					}
					if (ImGui::MenuItem("Universal", "T", isUniversal)) {
						state.operation = Operation::UNIVERSAL;
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Local Mode", "Space", isLocal)) {
						state.mode = (state.mode == Mode::LOCAL) ? Mode::WORLD : Mode::LOCAL;
					}
					ImGui::Separator();
					ImGui::MenuItem("Use Snapping", nullptr, &state.useSnap);
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
		}
	};

} // namespace Utils