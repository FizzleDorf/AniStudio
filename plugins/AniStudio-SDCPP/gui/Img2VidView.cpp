// Img2VidView.cpp
#include "Img2VidView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Img2VidView::GetDefaultComponents() const {
        return {
            "DiffusionModel",
            "Vae",
            "AudioVae",
            "LlmEncoder",
            "LlmVision",
            "Latent",
            "Sampler",
            "HighNoiseSampler",
            "VideoParams",
            "Guidance",
            "Prompt",
            "OutputImage",
            "InputImage",
            "RefVideo",
            "RefAudio"
        };
    }

    std::vector<std::string> Img2VidView::GetFilteredComponents() const {
        return {
            "Checkpoint",
            "ClipL",
            "ClipG",
            "ClipVision",
            "T5XXL",
            "VaeTiling",
            "Taesd",
            "OutputVideo",
            "Lora",
            "ControlNet",
            "Embeddings",
            "RefImages",
            "ControlFrames",
            "PhotoMaker",
            "StackedIdEmbed",
            "Conversion",
            "Esrgan",
            "HighNoiseDiffusionModel",
            "EasyCache"
        };
    }

} // namespace GUI