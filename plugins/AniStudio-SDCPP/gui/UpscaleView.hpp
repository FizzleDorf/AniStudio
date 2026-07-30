// UpscaleView.hpp
#pragma once

#include "BaseDiffusionView.hpp"

namespace GUI {

    class UpscaleView : public BaseDiffusionView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Upscale View",
            "category": "Diffusion",
            "description": "Upscale images with ESRGAN models."
        })";
        }

        UpscaleView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseDiffusionView(mgr, vm) {
            viewName = "UpscaleView";
            windowOpen = true;
        }

        std::string GetTaskType() const override { return "Upscaling"; }

    protected:
        std::vector<std::string> GetDefaultComponents() const override;
        bool UseStateActiveSeparation() const override { return false; }
    };

} // namespace GUI