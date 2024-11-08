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
    bool vae_decode_only = false;
};

struct TaesdComponent : public ECS::BaseComponent {
    std::string taesdPath = "";
    std::string taesdName = "<none>";
    bool isEncoderLoaded = false;
};
} // namespace ECS