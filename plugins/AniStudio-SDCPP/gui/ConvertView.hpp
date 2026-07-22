#pragma once

#include "DiffusionOptions.hpp"
#include "ECS.h"
#include "ViewManager.hpp"
#include "SDCPPComponents.h"
#include "SDcppSystem.hpp"
#include "ConversionComponent.hpp"
#include "pch.h"
#include <memory>
#include <unordered_map>

namespace GUI {

	class ConvertView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Convert Model",
            "category": "Diffusion",
            "description": "Convert diffusion models to GGUF/Quant format."
        })";
		}

		ConvertView::ConvertView(ECS::EntityManager& mgr, ViewManager& vm)
			: BaseView(mgr, vm)
		{
			viewName = "ConvertView";
			windowOpen = true;
		}
		~ConvertView() = default;

		void Init() override;
		void Render() override;

	private:
		// Entity for conversion
		ECS::EntityID convertEntity = 0;

		// UI state
		bool windowOpen = true;
		int numQueues = 1;
		bool isPaused = false;

		// Component visibility tracking
		mutable std::unordered_map<std::string, bool> componentVisibility;

		void InitializeEntity();
		void RenderEntityComponents();
		void RenderComponent(ECS::ComponentTypeID compId, const std::string& componentName);
		void Convert();
		void RenderQueueList();
		std::vector<std::string> GetCategoryRenderOrder() const;
	};

} // namespace GUI