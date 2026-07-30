// EditView.hpp
#pragma once

#include "BaseDiffusionView.hpp"

namespace GUI {

    class EditView : public BaseDiffusionView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Edit",
            "category": "Diffusion",
            "description": "Edit images using diffusion."
        })";
        }

        EditView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseDiffusionView(mgr, vm) {
            viewName = "EditView";
            windowOpen = true;
        }

        std::string GetTaskType() const override { return "Edit"; }

    protected:
        std::vector<std::string> GetDefaultComponents() const override;
        std::vector<std::string> GetDefaultVisibleComponents() const override;
    };

} // namespace GUI