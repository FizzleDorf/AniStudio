#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct T5XXLComponent : public ECS::BaseComponent {
    std::string encoderPath = "";
    std::string encoderName = "model.gguf";
    bool isEncoderLoaded = false;
};
}