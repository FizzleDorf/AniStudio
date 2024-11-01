#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct CLipLComponent : public ECS::BaseComponent {
    std::string encoderPath = "";
    std::string encoderName = "model.gguf";
    bool isEncoderLoaded = false;
};
}