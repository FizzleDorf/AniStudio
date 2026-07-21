#pragma once

#include "BaseView.hpp"
#include "TransformComponent.hpp"
#include "MeshComponent.hpp" 
#include "CameraComponent.hpp"
#include "ImGuizmo.h"
#include <vector>
#include <memory>

namespace GUI {

	class ModelView : public BaseView {
	public:
		ModelView(ECS::EntityManager& mgr, ViewManager& vm)
			: BaseView(mgr, vm) {
			viewName = "ModelView";
		}
		~ModelView() = default;

		void Init() override;
		void Update(float deltaTime) override;
		void Render() override;

		static const char* GetMetadataJSON() {
			return R"({
                "displayName": "Model View",
                "category": "3D",
                "description": "3D model viewer and editor"
            })";
		}

	private:
		// 3D Scene entities
		ECS::EntityID cameraEntity = UINT32_MAX;
		std::vector<ECS::EntityID> sceneObjects;
		std::vector<ECS::EntityID> selectedEntities;

		// Gizmo state
		ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
		ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
		bool useSnap = false;
		float snapValues[3] = { 1.0f, 1.0f, 1.0f };

		// Viewport state
		bool isViewportFocused = false;
		bool isViewportHovered = false;
		ImVec2 lastMousePos = { 0, 0 };
		bool firstMouse = true;

		// UI state
		bool showSceneHierarchy = true;
		bool showObjectInspector = true;
		bool showGizmoSettings = false;

		// Core functionality
		void CreateNewCube();
		void CreateCubeMeshData(ECS::MeshComponent& mesh);
		void UpdateCameraAspectRatio(float aspectRatio);
		void HandleCameraInput(float deltaTime);

		// Rendering methods
		void RenderMenuBar();
		void RenderViewport(const ImVec2& size, const ImVec2& pos);
		void RenderSimple3DObjects(const ImVec2& viewportPos, const ImVec2& viewportSize);
		void RenderGizmos(const ImVec2& viewportPos, const ImVec2& viewportSize);
		void RenderGizmoSettings();
		void RenderSceneHierarchy();
		void RenderObjectInspector();

		// Input handling
		void HandleViewportInput();

		// Utility methods
		ImVec2 Project3DToScreen(const glm::vec3& worldPos, const glm::mat4& view,
			const glm::mat4& projection, const ImVec2& viewportPos, const ImVec2& viewportSize);
		void DrawCubeWireframe(const ImVec2& viewportPos, const ImVec2& viewportSize,
			const ECS::TransformComponent& transform, ImU32 color, bool isSelected);

		// Safe entity operations
		bool IsEntitySafe(ECS::EntityID entity) const;
		void CleanupInvalidEntities();
	};

} // namespace GUI