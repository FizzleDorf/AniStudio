#pragma once

#include "BaseView.hpp"
#include "components.h"
#include "systems.h"
#include <unordered_map>
#include <functional>

namespace GUI {

	struct ProgressData {
		int currentStep = 0;
		int totalSteps = 0;
		float currentTime = 0.0f;
		bool isProcessing = false;
	};

	class DiffusionView : public BaseView {
	public:
		DiffusionView(ECS::EntityManager& entityMgr);
		virtual ~DiffusionView();

		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "Diffusion",
                "category": "Generation",
                "description": "AI image generation using Stable Diffusion models."
            })";
		}

		static ViewMetadata GetMetadata() {
			return GetMetadataFor<DiffusionView>();
		}

		void Init() override;
		void Render() override;

		// Serialization
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json& j) override;

	private:
		// Entity management
		ECS::EntityID txt2imgEntity = 0;
		ECS::EntityID img2imgEntity = 0;
		bool isTxt2ImgMode = true;

		// Component visibility management
		std::unordered_map<std::string, bool> componentVisibility;
		void InitializeComponentVisibility();

		// UI rendering methods
		void RenderEntityComponents(const ECS::EntityID entity);
		void RenderComponentWithCheckbox(const ECS::EntityID entity, const std::string& componentName,
			const std::string& displayName, const std::function<void()>& renderFunc);
		void RenderComponentSchema(const ECS::EntityID entity, const std::string& componentName, ECS::BaseComponent* component);
		void RenderQueueList();
		void RenderMetadataControls();

		// Entity operations
		void ResetEntities();
		bool IsEntitySafeToUse(ECS::EntityID entity) const;
		void UpdateModelPath(const ECS::EntityID entity, const std::string& componentName);

		// Event handling
		void HandleT2IEvent();
		void HandleI2IEvent();
		void HandleUpscaleEvent();

		// Metadata operations
		void SaveMetadataToJson(const std::string& filepath);
		void LoadMetadataFromJson(const std::string& filepath);
		void LoadMetadataFromPNG(const std::string& imagePath);

		// Queue management
		int numQueues = 1;
		bool isPaused = false;
	};

} // namespace GUI