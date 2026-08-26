// Img2ImgView.hpp
#pragma once

#include "BaseDiffusionView.hpp"

namespace GUI {

    class Img2ImgView : public BaseDiffusionView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Img2Img",
            "category": "Diffusion",
            "description": "Generate images from image input."
        })";
        }

        Img2ImgView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseDiffusionView(mgr, vm) {
            viewName = "Img2ImgView";
            windowOpen = true;
        }

        std::string GetTaskType() const override { return "Img2Img"; }

    protected:
        std::vector<std::string> GetDefaultComponents() const override;
        std::vector<std::string> GetFilteredComponents() const override;
    };

} // namespace GUI