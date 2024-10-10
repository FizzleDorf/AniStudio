#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"

namespace ECS {
struct CFGComponent : public ECS::BaseComponent {

    float cfg = 7.0;
};
}
