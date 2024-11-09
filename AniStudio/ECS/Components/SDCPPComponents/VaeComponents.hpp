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

    VaeComponent &operator=(const VaeComponent &other) {
        if (this != &other) {
            vaePath = other.vaePath;
            vaeName = other.vaeName;
            isEncoderLoaded = other.isEncoderLoaded;
            isTiled = other.isTiled;
            keep_vae_on_cpu = other.keep_vae_on_cpu;
            vae_decode_only = other.vae_decode_only;
        }
        return *this;
    }
};

struct TaesdComponent : public ECS::BaseComponent {
    std::string taesdPath = "";
    std::string taesdName = "<none>";
    bool isEncoderLoaded = false;

    TaesdComponent &operator=(const TaesdComponent &other) {
        if (this != &other) {
            taesdPath = other.taesdPath;
            taesdName = other.taesdName;
            isEncoderLoaded = other.isEncoderLoaded;
        }
        return *this;
    }
};
} // namespace ECS