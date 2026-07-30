// Txt2ImgView.cpp
#include "Txt2ImgView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Txt2ImgView::GetDefaultComponents() const {
        return {
            "CheckpointComponent",
            "LatentComponent",
            "SamplerComponent",
            "GuidanceComponent",
            "PromptComponent",
            "OutputImageComponent"
        };
    }

    std::vector<std::string> Txt2ImgView::GetDefaultVisibleComponents() const {
        return {
            "CheckpointComponent",
            "LatentComponent",
            "SamplerComponent",
            "GuidanceComponent",
            "PromptComponent",
            "OutputImageComponent"
        };
    }

} // namespace GUI