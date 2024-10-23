#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct TaesdComponent : public ECS::BaseComponent {
    std::string taesdPath = "path/to/model";
    std::string taesdName = "model.gguf";
    bool isEncoderLoaded = false;
};
} // namespace ECS