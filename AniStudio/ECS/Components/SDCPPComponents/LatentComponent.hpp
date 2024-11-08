#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct LatentComponent : public ECS::BaseComponent {
    // unsigned char *latentData = nullptr;
    int latentWidth = 512;
    int latentHeight = 512;
    int batchSize = 1;

    LatentComponent &operator=(const LatentComponent &other) {
        if (this != &other) { // Self-assignment check
            latentWidth = other.latentWidth;
            latentHeight = other.latentHeight;
            batchSize = other.batchSize;
        }
        return *this;
    }
};
}
