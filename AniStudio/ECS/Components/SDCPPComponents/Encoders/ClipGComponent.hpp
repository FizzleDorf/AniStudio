#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct CLipGComponent : public ECS::BaseComponent {
    std::string encoderPath = "";
    std::string encoderName = "model.gguf";
    bool isEncoderLoaded = false;
};
} // namespace ECS
