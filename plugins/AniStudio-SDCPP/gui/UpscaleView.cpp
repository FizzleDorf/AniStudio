// UpscaleView.cpp
#include "UpscaleView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> UpscaleView::GetDefaultComponents() const {
        return {
            "InputImage",
            "OutputImage",
            "Esrgan"
        };
    }

    std::vector<std::string> UpscaleView::GetFilteredComponents() const {
        return {
            "Checkpoint",
            "DiffusionModel",
            "ClipL",
            "ClipG",
            "T5XXL",
            "ClipVision",
            "LlmEncoder",
            "LlmVision",
            "Vae",
            "VaeTiling",
            "Taesd",
            "AudioVae",
            "Latent",
            "Sampler",
            "Guidance",
            "Prompt",
            "InputVideo",
            "OutputVideo",
            "Lora",
            "ControlNet",
            "Embeddings",
            "RefImages",
            "ControlFrames",
            "RefVideo",
            "RefAudio",
            "PhotoMaker",
            "StackedIdEmbed",
            "Conversion",
            "HighNoiseDiffusionModel",
            "HighNoiseSampler",
            "VideoParams",
            "EasyCache"
        };
    }

} // namespace GUI