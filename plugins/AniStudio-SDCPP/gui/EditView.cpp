// EditView.cpp
#include "EditView.hpp"

using namespace ECS;

namespace GUI {

    std::vector<std::string> EditView::GetDefaultComponents() const {
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

    std::vector<std::string> EditView::GetFilteredComponents() const {
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