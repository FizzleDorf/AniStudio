#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct LatentComponent : public ECS::BaseComponent {
    unsigned char *imageData = nullptr;
    int width = 0;
    int height = 0;
};
}
