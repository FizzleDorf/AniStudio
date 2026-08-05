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
            "AudioVaeComponent",
            "LlmEncoderComponent",
            "LlmVisionComponent",
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
            "EsrganComponent",
            "RefVideoComponent",
            "RefAudioComponent",
            "RefVideoAudioComponent"
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
            "AudioVaeComponent",
            "LlmEncoderComponent",
            "LlmVisionComponent",
            "LatentComponent",
            "SamplerComponent",
            "HighNoiseSamplerComponent",
            "VideoParamsComponent",
            "GuidanceComponent",
            "PromptComponent",
            "OutputImageComponent",
            "InputImageComponent",
            "RefVideoComponent",
            "RefAudioComponent"
        };
    }

} // namespace GUI