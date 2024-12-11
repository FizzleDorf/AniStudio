#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct EsrganComponent : public ECS::BaseModelComponent {
    float scale = 1.5;
};
}
