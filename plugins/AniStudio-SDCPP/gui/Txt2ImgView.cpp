// Txt2ImgView.cpp
#include "Txt2ImgView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Txt2ImgView::GetDefaultComponents() const {
        return {
            "Checkpoint",
            "Latent",
            "Sampler",
            "Guidance",
            "Prompt",
            "OutputImage"
        };
    }

    std::vector<std::string> Txt2ImgView::GetFilteredComponents() const {
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
            "HighNoiseSampler"
        };
    }

} // namespace GUI