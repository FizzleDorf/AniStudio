// EditView.cpp
#include "EditView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> EditView::GetDefaultComponents() const {
        return {
            "CheckpointComponent",
            "LatentComponent",
            "SamplerComponent",
            "GuidanceComponent",
            "PromptComponent",
            "OutputImageComponent",
            "InputImageComponent"
        };
    }

    std::vector<std::string> EditView::GetDefaultVisibleComponents() const {
        return {
            "CheckpointComponent",
            "LatentComponent",
            "SamplerComponent",
            "GuidanceComponent",
            "PromptComponent",
            "OutputImageComponent",
            "InputImageComponent"
        };
    }

} // namespace GUI