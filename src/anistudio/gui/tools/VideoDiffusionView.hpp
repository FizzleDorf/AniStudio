/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

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