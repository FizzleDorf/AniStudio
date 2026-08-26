// Txt2VidView.cpp
#include "Txt2VidView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Txt2VidView::GetDefaultComponents() const {
        return {
            "DiffusionModel",
            "Vae",
            "AudioVae",
            "LlmEncoder",
            "Latent",
            "Sampler",
            "Guidance",
            "Prompt",
            "VideoParams",
            "OutputImage"
        };
    }

    std::vector<std::string> Txt2VidView::GetFilteredComponents() const {
        return {
            "Checkpoint",
            "ClipL",
            "ClipG",
            "ClipVision",
            "T5XXL",
            "LlmVision",
            "VaeTiling",
            "Taesd",
            "InputImage",
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
            "Esrgan",
            "HighNoiseDiffusionModel",
            "HighNoiseSampler",
            "EasyCache"
        };
    }

} // namespace GUI