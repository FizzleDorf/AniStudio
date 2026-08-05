// Txt2VidView.hpp
#pragma once

#include "BaseDiffusionView.hpp"

namespace GUI {

    class Txt2VidView : public BaseDiffusionView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Txt2Vid",
            "category": "Diffusion",
            "description": "Generate video from text prompts."
        })";
        }

        Txt2VidView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseDiffusionView(mgr, vm) {
            viewName = "Txt2VidView";
            windowOpen = true;
        }

        std::string GetTaskType() const override { return "Img2Vid"; }

    protected:
        std::vector<std::string> GetDefaultComponents() const override;
        std::vector<std::string> GetDefaultVisibleComponents() const override;
    };

} // namespace GUI