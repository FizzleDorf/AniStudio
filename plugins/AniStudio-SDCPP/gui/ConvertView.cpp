// ConvertView.cpp
#include "ConvertView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> ConvertView::GetDefaultComponents() const {
        return {
            "Checkpoint",
            "Vae",
            "Conversion"
        };
    }

    std::vector<std::string> ConvertView::GetFilteredComponents() const {
        return {
            "InputImage",
            "InputVideo",
            "OutputVideo",
            "RefVideo",
            "RefAudio",
            "PhotoMaker",
            "StackedIdEmbed",
            "VideoParams",
            "AudioVae",
            "LlmEncoder",
            "LlmVision",
            "HighNoiseDiffusionModel",
            "HighNoiseSampler",
            "Latent",
            "Sampler",
            "Guidance",
            "Prompt",
            "OutputImage",
            "Lora",
            "ControlNet",
            "Embeddings",
            "RefImages",
            "ControlFrames",
            "Esrgan",
            "EasyCache"
        };
    }

} // namespace GUI