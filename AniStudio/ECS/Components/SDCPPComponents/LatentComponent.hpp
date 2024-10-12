#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct LatentComponent : public ECS::BaseComponent {
    unsigned char *latentData = nullptr;
    int latentWidth = 0;
    int latentHeight = 0;
};
}
