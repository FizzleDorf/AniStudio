// Txt2VidView.cpp
#include "Txt2VidView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Txt2VidView::GetDefaultComponents() const {
        return {
            "DiffusionModelComponent",
            "VaeComponent",
            "AudioVaeComponent",
            "LlmEncoderComponent",
            "LatentComponent",
            "SamplerComponent",
            "GuidanceComponent",
            "PromptComponent",
            "VideoParamsComponent",
            "OutputImageComponent"
        };
    }

    std::vector<std::string> Txt2VidView::GetDefaultVisibleComponents() const {
        return {
            "DiffusionModelComponent",
            "VaeComponent",
            "AudioVaeComponent",
            "LlmEncoderComponent",
            "LatentComponent",
            "SamplerComponent",
            "GuidanceComponent",
            "PromptComponent",
            "VideoParamsComponent",
            "OutputImageComponent"
        };
    }

} // namespace GUI