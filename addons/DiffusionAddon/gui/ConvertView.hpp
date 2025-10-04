#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "GUI.h"
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
            "category": "Tools",
            "description": "Convert diffusion models to GGUF/Quant format."
        })";
	}

    ConvertView(ECS::EntityManager &entityMgr);
    ~ConvertView() = default;

    void Init() override;
    void Render() override;

private:
    ECS::SamplerComponent samplerComp;
    ECS::ModelComponent modelComp;
    ECS::VaeComponent vaeComp;
    
    void Convert();
    void RenderVaeLoader();
};

} // namespace GUI