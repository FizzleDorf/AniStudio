#pragma once

#include "BaseComponent.hpp"

namespace ECS {
struct CFGComponent : public ECS::BaseComponent {
    float cfg = 7.0;
    float guidance = 2.0f;
};
}
