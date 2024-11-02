#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct TaesdComponent : public ECS::BaseComponent {
    std::string taesdPath = "";
    std::string taesdName = "<none>";
    bool isEncoderLoaded = false;
};
} // namespace ECS