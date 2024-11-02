#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct VaeComponent : public ECS::BaseComponent {
    std::string vaePath = "";
    std::string vaeName = "<none>";
    bool isEncoderLoaded = false;
    bool isTiled = false;
};
} // namespace ECS