#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct VaeComponent : public ECS::BaseComponent {
    std::string vaePath = "";
    std::string vaeName = "model.gguf";
    bool isEncoderLoaded = false;
    bool isTiled = false;
    bool keep_vae_on_cpu = true;
};
} // namespace ECS