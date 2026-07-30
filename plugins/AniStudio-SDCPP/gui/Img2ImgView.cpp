// Img2ImgView.cpp
#include "Img2ImgView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Img2ImgView::GetDefaultComponents() const {
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

    std::vector<std::string> Img2ImgView::GetDefaultVisibleComponents() const {
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