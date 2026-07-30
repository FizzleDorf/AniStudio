// Img2VidView.cpp
#include "Img2VidView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Img2VidView::GetDefaultComponents() const {
        return {
            "CheckpointComponent",
            "ClipLComponent",
            "ClipGComponent",
            "ClipVisionComponent",
            "T5XXLComponent",
            "DiffusionModelComponent",
            "HighNoiseDiffusionModelComponent",
            "VaeComponent",
            "LoraComponent",
            "TaesdComponent",
            "LatentComponent",
            "SamplerComponent",
            "HighNoiseSamplerComponent",
            "VideoParamsComponent",
            "GuidanceComponent",
            "PromptComponent",
            "OutputImageComponent",
            "InputImageComponent",
            "EndImageComponent",
            "ControlNetComponent",
            "EmbeddingComponent",
            "EsrganComponent"
        };
    }

    std::vector<std::string> Img2VidView::GetDefaultVisibleComponents() const {
        return {
            "CheckpointComponent",
            "ClipLComponent",
            "ClipGComponent",
            "ClipVisionComponent",
            "T5XXLComponent",
            "DiffusionModelComponent",
            "HighNoiseDiffusionModelComponent",
            "VaeComponent",
            "LatentComponent",
            "SamplerComponent",
            "HighNoiseSamplerComponent",
            "VideoParamsComponent",
            "GuidanceComponent",
            "PromptComponent",
            "OutputImageComponent",
            "InputImageComponent"
        };
    }

} // namespace GUI