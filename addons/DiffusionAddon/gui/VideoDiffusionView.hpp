#pragma once

#include "BaseView.hpp"
#include "SDCPPComponents.h"
#include "SDcppSystem.hpp"
#include <unordered_map>
#include <functional>

namespace GUI {

	class VideoDiffusionView : public BaseView {
	public:
		VideoDiffusionView(ECS::EntityManager& entityMgr);
		virtual ~VideoDiffusionView();

		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "Video Diffusion",
                "category": "Diffusion",
                "description": "AI video generation using stable diffusion video models."
            })";
		}

		static ViewMetadata GetMetadata() {
			return GetMetadataFor<VideoDiffusionView>();
		}

		void Init() override;
		void Update(float deltaT) override;
		void Render() override;

		// Serialization
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json& j) override;

	private:
		// Entity management - Only Img2Vid now (Edit moved to DiffusionView)
		ECS::EntityID img2vidEntity = 0;

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

		// Event handling - Only Img2Vid now
		void HandleImg2VidEvent();

		// Metadata operations
		void SaveMetadataToJson(const std::string& filepath);
		void LoadMetadataFromJson(const std::string& filepath);
		void LoadMetadataFromVideo(const std::string& videoPath);

		// Queue management
		int numQueues = 1;
		bool isPaused = false;
	};

} // namespace GUI