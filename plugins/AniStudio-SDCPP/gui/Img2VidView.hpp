// Img2VidView.hpp
#pragma once

#include "BaseDiffusionView.hpp"

namespace GUI {

    class Img2VidView : public BaseDiffusionView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Img2Vid",
            "category": "Diffusion",
            "description": "Generate video from image input."
        })";
        }

        Img2VidView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseDiffusionView(mgr, vm) {
            viewName = "Img2VidView";
            windowOpen = true;
        }

        std::string GetTaskType() const override { return "Img2Vid"; }

    protected:
        std::vector<std::string> GetDefaultComponents() const override;
        std::vector<std::string> GetFilteredComponents() const override;
    };

} // namespace GUI