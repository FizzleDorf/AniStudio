#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct EncoderComponent : public ECS::BaseComponent {
    std::string encoderPath = "path/to/model";
    std::string encoderName = "model.gguf";
    bool isEncoderLoaded = false;
};
}