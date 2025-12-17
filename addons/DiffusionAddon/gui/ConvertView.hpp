#pragma once

#include "DiffusionOptions.hpp"
#include "ECS.h"
#include "ViewManager.hpp"
#include "FilePaths.hpp"
#include "ImGuiFileDialog.h"
#include "SDCPPComponents.h"
#include "SDcppSystem.hpp"
#include "pch.h"

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

		ConvertView(ECS::EntityManager &entityMgr, ImGuiContext* mainContext = nullptr);
		~ConvertView() = default;

		void Init() override;
		void Render() override;

	private:
		ECS::SamplerComponent samplerComp;
		ECS::ModelComponent modelComp;
		ECS::VaeComponent vaeComp;

		void Convert();
		void RenderVaeLoader();
		void RenderQueueList();
	};

} // namespace GUI