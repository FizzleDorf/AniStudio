// ConvertView.hpp
#pragma once

#include "BaseDiffusionView.hpp"

namespace GUI {

    class ConvertView : public BaseDiffusionView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Convert Model",
            "category": "Diffusion",
            "description": "Convert diffusion models to GGUF/Quant format."
        })";
        }

        ConvertView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseDiffusionView(mgr, vm) {
            viewName = "ConvertView";
            windowOpen = true;
        }

        std::string GetTaskType() const override { return "Conversion"; }

    protected:
        std::vector<std::string> GetDefaultComponents() const override;
        bool UseStateActiveSeparation() const override { return false; }
    };

} // namespace GUI