// Txt2ImgView.hpp
#pragma once

#include "BaseDiffusionView.hpp"

namespace GUI {

    class Txt2ImgView : public BaseDiffusionView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Txt2Img",
            "category": "Diffusion",
            "description": "Generate images from text prompts."
        })";
        }

        Txt2ImgView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseDiffusionView(mgr, vm) {
            viewName = "Txt2ImgView";
            windowOpen = true;
        }

        std::string GetTaskType() const override { return "Inference"; }

    protected:
        std::vector<std::string> GetDefaultComponents() const override;
        std::vector<std::string> GetDefaultVisibleComponents() const override;
    };

} // namespace GUI