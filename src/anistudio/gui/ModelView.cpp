#include "ModelView.hpp"
#include <iostream>
#include <algorithm>
#include "Events.hpp"

namespace GUI {

	void ModelView::Init() {
		std::cout << "[ModelView] Initializing..." << std::endl;

		try {
			// Initialize ImGuizmo context - this is crucial!
			ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

			// Create camera entity with proper error checking
			cameraEntity = m_entityManager.AddNewEntity();
			std::cout << "[ModelView] Created camera entity: " << cameraEntity << std::endl;

			// Add components with validation
			if (m_entityManager.IsEntityValid(cameraEntity)) {
				auto& transform = m_entityManager.AddComponent<ECS::TransformComponent>(cameraEntity);
				auto& camera = m_entityManager.AddComponent<ECS::CameraComponent>(cameraEntity);

				transform.SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
				camera.SetAspectRatio(16.0f / 9.0f);

				std::cout << "[ModelView] Camera components added successfully" << std::endl;
			}

			// Create a sample 3D object
			CreateNewCube();

			std::cout << "[ModelView] Initialization complete" << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error during initialization: " << e.what() << std::endl;
		}
	}

	void ModelView::Update(float deltaTime) {
		try {
			// Clean up any invalid entities first
			CleanupInvalidEntities();

			// Safe camera input handling
			HandleCameraInput(deltaTime);

			// Begin ImGuizmo frame - this is required!
			ImGuizmo::BeginFrame();
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error in Update: " << e.what() << std::endl;
		}
	}

	void ModelView::Render() {
		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {

			if (!windowOpen) {
				std::unordered_map<std::string, std::any> eventData;
				eventData["workspaceID"] = GetID();
				eventData["viewTypeName"] = viewName;
				ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
				ImGui::End();
			}

			RenderMenuBar();

			// Get available content area
			ImVec2 contentRegion = ImGui::GetContentRegionAvail();
			ImVec2 viewportPos = ImGui::GetCursorScreenPos();

			// Update camera aspect ratio safely
			UpdateCameraAspectRatio(contentRegion.x / contentRegion.y);

			// Render the 3D viewport
			RenderViewport(contentRegion, viewportPos);

			// Handle viewport input safely
			HandleViewportInput();

			// Render side panels
			RenderSceneHierarchy();
			RenderObjectInspector();
			if (showGizmoSettings) {
				RenderGizmoSettings();
			}
		}
		ImGui::End();
	}

	void ModelView::RenderGizmoSettings() {
		if (!showGizmoSettings) return;

		if (ImGui::Begin("Gizmo Settings", &showGizmoSettings)) {
			// Operation selection
			ImGui::Text("Operation:");

			bool isTranslate = (currentGizmoOperation == ImGuizmo::TRANSLATE);
			bool isRotate = (currentGizmoOperation == ImGuizmo::ROTATE);
			bool isScale = (currentGizmoOperation == ImGuizmo::SCALE);
			bool isUniversal = (currentGizmoOperation == ImGuizmo::UNIVERSAL);

			if (ImGui::RadioButton("Translate (T)", isTranslate)) {
				currentGizmoOperation = ImGuizmo::TRANSLATE;
			}
			if (ImGui::RadioButton("Rotate (R)", isRotate)) {
				currentGizmoOperation = ImGuizmo::ROTATE;
			}
			if (ImGui::RadioButton("Scale (S)", isScale)) {
				currentGizmoOperation = ImGuizmo::SCALE;
			}
			if (ImGui::RadioButton("Universal (U)", isUniversal)) {
				currentGizmoOperation = ImGuizmo::UNIVERSAL;
			}

			ImGui::Separator();

			// Mode selection
			ImGui::Text("Mode:");

			bool isLocal = (currentGizmoMode == ImGuizmo::LOCAL);
			bool isWorld = (currentGizmoMode == ImGuizmo::WORLD);

			if (ImGui::RadioButton("Local", isLocal)) {
				currentGizmoMode = ImGuizmo::LOCAL;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("World", isWorld)) {
				currentGizmoMode = ImGuizmo::WORLD;
			}

			ImGui::Separator();

			// Snapping settings
			ImGui::Checkbox("Use Snapping", &useSnap);
			if (useSnap) {
				ImGui::Text("Snap Values:");

				if (currentGizmoOperation == ImGuizmo::TRANSLATE) {
					ImGui::DragFloat3("Translation Snap", snapValues, 0.1f, 0.1f, 10.0f);
				}
				else if (currentGizmoOperation == ImGuizmo::ROTATE) {
					ImGui::DragFloat("Rotation Snap (degrees)", &snapValues[0], 1.0f, 1.0f, 90.0f);
				}
				else if (currentGizmoOperation == ImGuizmo::SCALE) {
					ImGui::DragFloat("Scale Snap", &snapValues[0], 0.01f, 0.01f, 1.0f);
				}
			}

			ImGui::Separator();

			// Status display
			ImGui::Text("Status:");
			const char* usingText = ImGuizmo::IsUsing() ? "Yes" : "No";
			const char* overText = ImGuizmo::IsOver() ? "Yes" : "No";
			ImGui::Text("Using Gizmo: %s", usingText);
			ImGui::Text("Over Gizmo: %s", overText);

			ImGui::Separator();

			// Additional gizmo controls
			ImGui::Text("Controls:");
			ImGui::BulletText("T - Translate mode");
			ImGui::BulletText("R - Rotate mode");
			ImGui::BulletText("S - Scale mode");
			ImGui::BulletText("U - Universal mode");
			ImGui::BulletText("Space - Toggle Local/World mode");
		}
		ImGui::End();
	}

	void ModelView::CreateNewCube() {
		try {
			ECS::EntityID cubeEntity = m_entityManager.AddNewEntity();
			std::cout << "[ModelView] Created cube entity: " << cubeEntity << std::endl;

			if (m_entityManager.IsEntityValid(cubeEntity)) {
				// Add transform component
				auto& transform = m_entityManager.AddComponent<ECS::TransformComponent>(cubeEntity);
				transform.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));

				// Add mesh component
				auto& mesh = m_entityManager.AddComponent<ECS::MeshComponent>(cubeEntity);
				mesh.color = glm::vec3(0.7f, 0.3f, 0.8f); // Purple color

				// Create simple cube data (no OpenGL setup for now)
				CreateCubeMeshData(mesh);

				sceneObjects.push_back(cubeEntity);
				std::cout << "[ModelView] Cube created successfully with " << mesh.vertices.size() << " vertices" << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error creating cube: " << e.what() << std::endl;
		}
	}

	void ModelView::CreateCubeMeshData(ECS::MeshComponent& mesh) {
		// Simple cube vertices - just data, no OpenGL calls
		mesh.vertices = {
			// Front face
			{{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			{{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			{{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			{{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

			// Back face
			{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
			{{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
			{{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
			{{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}
		};

		mesh.indices = {
			// Front face
			0, 1, 2, 2, 3, 0,
			// Back face
			4, 5, 6, 6, 7, 4
		};
	}

	void ModelView::UpdateCameraAspectRatio(float aspectRatio) {
		try {
			if (m_entityManager.IsEntityValid(cameraEntity) && m_entityManager.HasComponent<ECS::CameraComponent>(cameraEntity)) {
				auto& camera = m_entityManager.GetComponent<ECS::CameraComponent>(cameraEntity);
				camera.SetAspectRatio(aspectRatio);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error updating camera aspect ratio: " << e.what() << std::endl;
		}
	}

	void ModelView::HandleCameraInput(float deltaTime) {
		if (!isViewportFocused) return;

		try {
			if (!m_entityManager.IsEntityValid(cameraEntity) || !m_entityManager.HasComponent<ECS::CameraComponent>(cameraEntity)) {
				return;
			}

			auto& camera = m_entityManager.GetComponent<ECS::CameraComponent>(cameraEntity);

			// Keyboard movement
			if (ImGui::IsKeyDown(ImGuiKey_W)) camera.ProcessKeyboard(0, deltaTime); // Forward
			if (ImGui::IsKeyDown(ImGuiKey_S)) camera.ProcessKeyboard(1, deltaTime); // Backward
			if (ImGui::IsKeyDown(ImGuiKey_A)) camera.ProcessKeyboard(2, deltaTime); // Left
			if (ImGui::IsKeyDown(ImGuiKey_D)) camera.ProcessKeyboard(3, deltaTime); // Right
			if (ImGui::IsKeyDown(ImGuiKey_Q)) camera.ProcessKeyboard(4, deltaTime); // Up
			if (ImGui::IsKeyDown(ImGuiKey_E)) camera.ProcessKeyboard(5, deltaTime); // Down

			// Mouse look (only when right mouse button is held)
			if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
				ImVec2 mousePos = ImGui::GetMousePos();

				if (!firstMouse) {
					float xOffset = mousePos.x - lastMousePos.x;
					float yOffset = lastMousePos.y - mousePos.y;

					camera.ProcessMouseMovement(xOffset, yOffset);
				}
				else {
					firstMouse = false;
				}

				lastMousePos = mousePos;
			}
			else {
				firstMouse = true;
			}

			// Mouse wheel zoom
			float wheel = ImGui::GetIO().MouseWheel;
			if (wheel != 0.0f) {
				camera.ProcessMouseScroll(wheel);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error in camera input: " << e.what() << std::endl;
		}
	}

	void ModelView::RenderMenuBar() {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("View")) {
				ImGui::MenuItem("Scene Hierarchy", nullptr, &showSceneHierarchy);
				ImGui::MenuItem("Object Inspector", nullptr, &showObjectInspector);
				ImGui::MenuItem("Gizmo Settings", nullptr, &showGizmoSettings);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create")) {
				if (ImGui::MenuItem("Add Cube")) {
					CreateNewCube();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Gizmo")) {
				// Operation selection
				if (ImGui::MenuItem("Translate", "T", currentGizmoOperation == ImGuizmo::TRANSLATE)) {
					currentGizmoOperation = ImGuizmo::TRANSLATE;
				}
				if (ImGui::MenuItem("Rotate", "R", currentGizmoOperation == ImGuizmo::ROTATE)) {
					currentGizmoOperation = ImGuizmo::ROTATE;
				}
				if (ImGui::MenuItem("Scale", "S", currentGizmoOperation == ImGuizmo::SCALE)) {
					currentGizmoOperation = ImGuizmo::SCALE;
				}
				if (ImGui::MenuItem("Universal", "U", currentGizmoOperation == ImGuizmo::UNIVERSAL)) {
					currentGizmoOperation = ImGuizmo::UNIVERSAL;
				}

				ImGui::Separator();

				// Mode selection
				if (ImGui::MenuItem("Local Mode", nullptr, currentGizmoMode == ImGuizmo::LOCAL)) {
					currentGizmoMode = ImGuizmo::LOCAL;
				}
				if (ImGui::MenuItem("World Mode", nullptr, currentGizmoMode == ImGuizmo::WORLD)) {
					currentGizmoMode = ImGuizmo::WORLD;
				}

				ImGui::Separator();
				ImGui::MenuItem("Use Snapping", nullptr, &useSnap);

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		// Handle hotkeys
		if (ImGui::IsKeyPressed(ImGuiKey_T)) currentGizmoOperation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed(ImGuiKey_R)) currentGizmoOperation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed(ImGuiKey_E)) currentGizmoOperation = ImGuizmo::SCALE; // E for scale like Blender
		if (ImGui::IsKeyPressed(ImGuiKey_U)) currentGizmoOperation = ImGuizmo::UNIVERSAL;
	}

	void ModelView::RenderViewport(const ImVec2& size, const ImVec2& pos) {
		// Create a colored rectangle as viewport
		ImGui::GetWindowDrawList()->AddRectFilled(
			pos,
			ImVec2(pos.x + size.x, pos.y + size.y),
			IM_COL32(40, 40, 40, 255)
		);

		// Visualize objects as simple shapes
		RenderSimple3DObjects(pos, size);

		// Add coordinate axes visualization
		float axisLength = 50.0f;
		ImVec2 axisCenter = ImVec2(pos.x + 60, pos.y + size.y - 60);

		// X axis (red)
		ImGui::GetWindowDrawList()->AddLine(
			axisCenter,
			ImVec2(axisCenter.x + axisLength, axisCenter.y),
			IM_COL32(255, 0, 0, 255), 3.0f
		);

		// Y axis (green) 
		ImGui::GetWindowDrawList()->AddLine(
			axisCenter,
			ImVec2(axisCenter.x, axisCenter.y - axisLength),
			IM_COL32(0, 255, 0, 255), 3.0f
		);

		// Z axis (blue) - diagonal to simulate perspective
		ImGui::GetWindowDrawList()->AddLine(
			axisCenter,
			ImVec2(axisCenter.x - axisLength * 0.7f, axisCenter.y - axisLength * 0.7f),
			IM_COL32(0, 0, 255, 255), 3.0f
		);

		// Make the viewport area interactive
		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton("ViewportButton", size);
		isViewportHovered = ImGui::IsItemHovered();
		isViewportFocused = ImGui::IsItemActive() || (isViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right));
	}

	void ModelView::RenderSimple3DObjects(const ImVec2& viewportPos, const ImVec2& viewportSize) {
		// Get camera for proper 3D projection
		if (!m_entityManager.IsEntityValid(cameraEntity) || !m_entityManager.HasComponent<ECS::CameraComponent>(cameraEntity)) {
			return;
		}

		try {
			auto& camera = m_entityManager.GetComponent<ECS::CameraComponent>(cameraEntity);
			auto& cameraTransform = m_entityManager.GetComponent<ECS::TransformComponent>(cameraEntity);

			glm::mat4 view = camera.GetViewMatrix();
			glm::mat4 projection = camera.GetProjectionMatrix();

			// Simple 3D to 2D projection for each object
			for (size_t i = 0; i < sceneObjects.size(); ++i) {
				ECS::EntityID entity = sceneObjects[i];

				if (!IsEntitySafe(entity)) {
					continue; // Skip invalid entities
				}

				if (m_entityManager.HasComponent<ECS::TransformComponent>(entity)) {
					auto& transform = m_entityManager.GetComponent<ECS::TransformComponent>(entity);

					// Get color from mesh component
					ImU32 color = IM_COL32(150, 75, 200, 255);
					if (m_entityManager.HasComponent<ECS::MeshComponent>(entity)) {
						auto& mesh = m_entityManager.GetComponent<ECS::MeshComponent>(entity);
						color = IM_COL32(
							(int)(mesh.color.r * 255),
							(int)(mesh.color.g * 255),
							(int)(mesh.color.b * 255),
							255
						);
					}

					// Check if selected
					bool isSelected = std::find(selectedEntities.begin(), selectedEntities.end(), entity) != selectedEntities.end();

					// Draw a 3D cube wireframe
					DrawCubeWireframe(viewportPos, viewportSize, transform, color, isSelected);

					// Add object label
					glm::vec3 pos = transform.position;
					ImVec2 screenPos = Project3DToScreen(pos, view, projection, viewportPos, viewportSize);

					std::string label = "Cube " + std::to_string(i);
					ImGui::GetWindowDrawList()->AddText(
						ImVec2(screenPos.x - 20, screenPos.y + 30),
						IM_COL32(255, 255, 255, 255),
						label.c_str()
					);
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error rendering 3D objects: " << e.what() << std::endl;
		}
	}

	ImVec2 ModelView::Project3DToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection,
		const ImVec2& viewportPos, const ImVec2& viewportSize) {
		// Transform world position to clip space
		glm::vec4 clipPos = projection * view * glm::vec4(worldPos, 1.0f);

		// Perform perspective division
		if (clipPos.w != 0.0f) {
			clipPos.x /= clipPos.w;
			clipPos.y /= clipPos.w;
		}

		// Convert from normalized device coordinates (-1 to 1) to screen coordinates
		float screenX = viewportPos.x + (clipPos.x + 1.0f) * 0.5f * viewportSize.x;
		float screenY = viewportPos.y + (1.0f - clipPos.y) * 0.5f * viewportSize.y;

		return ImVec2(screenX, screenY);
	}

	void ModelView::DrawCubeWireframe(const ImVec2& viewportPos, const ImVec2& viewportSize,
		const ECS::TransformComponent& transform, ImU32 color, bool isSelected) {
		if (!m_entityManager.IsEntityValid(cameraEntity)) return;

		try {
			auto& camera = m_entityManager.GetComponent<ECS::CameraComponent>(cameraEntity);
			glm::mat4 view = camera.GetViewMatrix();
			glm::mat4 projection = camera.GetProjectionMatrix();
			glm::mat4 model = transform.GetTransformMatrix();

			// Define cube vertices in local space
			std::vector<glm::vec3> cubeVertices = {
				// Front face
				{-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f},
				// Back face  
				{-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}
			};

			// Transform vertices to world space and then to screen space
			std::vector<ImVec2> screenVertices;
			for (const auto& vertex : cubeVertices) {
				glm::vec4 worldVertex = model * glm::vec4(vertex, 1.0f);
				ImVec2 screenPos = Project3DToScreen(glm::vec3(worldVertex), view, projection, viewportPos, viewportSize);
				screenVertices.push_back(screenPos);
			}

			// Define cube edges (which vertices connect to which)
			std::vector<std::pair<int, int>> edges = {
				// Front face edges
				{0, 1}, {1, 2}, {2, 3}, {3, 0},
				// Back face edges
				{4, 5}, {5, 6}, {6, 7}, {7, 4},
				// Connecting edges
				{0, 4}, {1, 5}, {2, 6}, {3, 7}
			};

			// Draw the wireframe
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			float lineThickness = isSelected ? 3.0f : 2.0f;
			ImU32 lineColor = isSelected ? IM_COL32(255, 255, 0, 255) : color;

			for (const auto& edge : edges) {
				const ImVec2& start = screenVertices[edge.first];
				const ImVec2& end = screenVertices[edge.second];

				// Only draw if both points are within reasonable screen bounds
				if (start.x > viewportPos.x - 100 && start.x < viewportPos.x + viewportSize.x + 100 &&
					start.y > viewportPos.y - 100 && start.y < viewportPos.y + viewportSize.y + 100 &&
					end.x > viewportPos.x - 100 && end.x < viewportPos.x + viewportSize.x + 100 &&
					end.y > viewportPos.y - 100 && end.y < viewportPos.y + viewportSize.y + 100) {

					drawList->AddLine(start, end, lineColor, lineThickness);
				}
			}

			// Draw vertices as small circles
			for (const auto& screenPos : screenVertices) {
				if (screenPos.x > viewportPos.x - 50 && screenPos.x < viewportPos.x + viewportSize.x + 50 &&
					screenPos.y > viewportPos.y - 50 && screenPos.y < viewportPos.y + viewportSize.y + 50) {

					drawList->AddCircleFilled(screenPos, 3.0f, lineColor);
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error drawing cube wireframe: " << e.what() << std::endl;
		}
	}

	void ModelView::HandleViewportInput() {
		// Handle object selection with safety checks
		if (isViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			selectedEntities.clear();

			// Clean up invalid entities before selection
			CleanupInvalidEntities();

			// Select first valid object if any exist
			if (!sceneObjects.empty()) {
				// Find the first valid entity
				for (ECS::EntityID entity : sceneObjects) {
					if (IsEntitySafe(entity)) {
						selectedEntities.push_back(entity);
						std::cout << "[ModelView] Selected entity: " << entity << std::endl;
						break;
					}
				}
			}
		}
	}

	void ModelView::RenderSceneHierarchy() {
		if (!showSceneHierarchy) return;

		if (ImGui::Begin("Scene Hierarchy", &showSceneHierarchy)) {
			ImGui::Text("3D Scene Objects:");
			ImGui::Separator();

			// List camera
			if (m_entityManager.IsEntityValid(cameraEntity)) {
				bool selected = std::find(selectedEntities.begin(), selectedEntities.end(), cameraEntity) != selectedEntities.end();
				if (ImGui::Selectable("Camera", selected)) {
					selectedEntities.clear();
					selectedEntities.push_back(cameraEntity);
				}
			}

			// Clean up invalid entities first
			CleanupInvalidEntities();

			// List scene objects
			for (size_t i = 0; i < sceneObjects.size(); ++i) {
				ECS::EntityID entity = sceneObjects[i];
				if (IsEntitySafe(entity)) {
					bool selected = std::find(selectedEntities.begin(), selectedEntities.end(), entity) != selectedEntities.end();
					std::string name = "Cube " + std::to_string(i);

					if (ImGui::Selectable(name.c_str(), selected)) {
						selectedEntities.clear();
						selectedEntities.push_back(entity);
						std::cout << "[ModelView] Selected entity from hierarchy: " << entity << std::endl;
					}

					// Right-click context menu
					if (ImGui::BeginPopupContextItem()) {
						if (ImGui::MenuItem("Delete")) {
							// Remove from selection if selected
							auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
							if (it != selectedEntities.end()) {
								selectedEntities.erase(it);
							}

							// Destroy entity
							m_entityManager.DestroyEntity(entity);

							// Remove from scene objects
							sceneObjects.erase(sceneObjects.begin() + i);

							std::cout << "[ModelView] Deleted entity: " << entity << std::endl;
						}
						ImGui::EndPopup();
					}
				}
			}

			ImGui::Separator();
			if (ImGui::Button("Add Cube")) {
				CreateNewCube();
			}
		}
		ImGui::End();
	}

	void ModelView::RenderObjectInspector() {
		if (!showObjectInspector) return;

		if (ImGui::Begin("Object Inspector", &showObjectInspector)) {
			if (selectedEntities.empty()) {
				ImGui::Text("No object selected");
			}
			else {
				ECS::EntityID selectedEntity = selectedEntities[0];

				if (!IsEntitySafe(selectedEntity)) {
					ImGui::Text("Invalid entity selected");
					selectedEntities.clear();
					ImGui::End();
					return;
				}

				ImGui::Text("Entity ID: %u", selectedEntity);

				try {
					// Show transform component if available
					if (m_entityManager.HasComponent<ECS::TransformComponent>(selectedEntity)) {
						auto& transform = m_entityManager.GetComponent<ECS::TransformComponent>(selectedEntity);

						ImGui::Separator();
						ImGui::Text("Transform");

						glm::vec3 pos = transform.position;
						if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
							transform.SetPosition(pos);
						}

						glm::vec3 rot = glm::degrees(transform.rotation);
						if (ImGui::DragFloat3("Rotation", &rot.x, 1.0f)) {
							transform.SetRotation(glm::radians(rot));
						}

						glm::vec3 scl = transform.scale;
						if (ImGui::DragFloat3("Scale", &scl.x, 0.1f, 0.1f)) {
							transform.SetScale(scl);
						}
					}

					// Show mesh component if available
					if (m_entityManager.HasComponent<ECS::MeshComponent>(selectedEntity)) {
						auto& mesh = m_entityManager.GetComponent<ECS::MeshComponent>(selectedEntity);

						ImGui::Separator();
						ImGui::Text("Mesh");

						ImGui::ColorEdit3("Color", &mesh.color.x);
						ImGui::Text("Vertices: %zu", mesh.vertices.size());
						ImGui::Text("Indices: %zu", mesh.indices.size());
					}

					// Show camera component if available
					if (m_entityManager.HasComponent<ECS::CameraComponent>(selectedEntity)) {
						auto& camera = m_entityManager.GetComponent<ECS::CameraComponent>(selectedEntity);

						ImGui::Separator();
						ImGui::Text("Camera");

						float fov = camera.fov;
						if (ImGui::SliderFloat("FOV", &fov, 1.0f, 120.0f)) {
							camera.SetFOV(fov);
						}

						ImGui::DragFloat("Near Plane", &camera.nearPlane, 0.01f, 0.01f, 10.0f);
						ImGui::DragFloat("Far Plane", &camera.farPlane, 1.0f, 1.0f, 1000.0f);
						ImGui::DragFloat("Movement Speed", &camera.movementSpeed, 0.1f, 0.1f, 10.0f);
						ImGui::DragFloat("Mouse Sensitivity", &camera.mouseSensitivity, 0.01f, 0.01f, 1.0f);
					}
				}
				catch (const std::exception& e) {
					ImGui::Text("Error accessing components: %s", e.what());
				}
			}
		}
		ImGui::End();
	}

	bool ModelView::IsEntitySafe(ECS::EntityID entity) const {
		try {
			return m_entityManager.IsEntityValid(entity);
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Exception checking entity safety: " << e.what() << std::endl;
			return false;
		}
	}

	void ModelView::CleanupInvalidEntities() {
		try {
			// Clean up scene objects
			auto it = std::remove_if(sceneObjects.begin(), sceneObjects.end(),
				[this](ECS::EntityID entity) {
				return !IsEntitySafe(entity);
			});

			if (it != sceneObjects.end()) {
				std::cout << "[ModelView] Cleaned up " << std::distance(it, sceneObjects.end()) << " invalid scene objects" << std::endl;
				sceneObjects.erase(it, sceneObjects.end());
			}

			// Clean up selected entities
			auto selectedIt = std::remove_if(selectedEntities.begin(), selectedEntities.end(),
				[this](ECS::EntityID entity) {
				return !IsEntitySafe(entity);
			});

			if (selectedIt != selectedEntities.end()) {
				std::cout << "[ModelView] Cleaned up " << std::distance(selectedIt, selectedEntities.end()) << " invalid selected entities" << std::endl;
				selectedEntities.erase(selectedIt, selectedEntities.end());
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ModelView] Error during cleanup: " << e.what() << std::endl;
		}
	}

} // namespace GUI