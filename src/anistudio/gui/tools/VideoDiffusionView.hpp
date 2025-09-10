#pragma once

#include "BaseView.hpp"
#include "components.h"
#include "systems.h"
#include <unordered_map>
#include <functional>

namespace GUI {

	class VideoDiffusionView : public BaseView {
	public:
		VideoDiffusionView(ECS::EntityManager& entityMgr);
		virtual ~VideoDiffusionView();

		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "Video Generation",
                "category": "Generation",
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
		// Entity management
		ECS::EntityID img2vidEntity = 0;
		ECS::EntityID editEntity = 0;
		bool isImg2VidMode = true;

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
		void HandleImg2VidEvent();
		void HandleEditEvent();

		// Metadata operations
		void SaveMetadataToJson(const std::string& filepath);
		void LoadMetadataFromJson(const std::string& filepath);
		void LoadMetadataFromVideo(const std::string& videoPath);

		// Queue management
		int numQueues = 1;
		bool isPaused = false;
	};

} // namespace GUI