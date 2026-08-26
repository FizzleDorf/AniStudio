// Img2ImgView.cpp
#include "Img2ImgView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> Img2ImgView::GetDefaultComponents() const {
        return {
            "Checkpoint",
            "Latent",
            "Sampler",
            "Guidance",
            "Prompt",
            "OutputImage",
            "InputImage"
        };
    }

    std::vector<std::string> Img2ImgView::GetFilteredComponents() const {
        return {
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