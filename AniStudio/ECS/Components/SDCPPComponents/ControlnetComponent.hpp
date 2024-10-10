#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "stb_image.h"
#include <string>

namespace ECS {
struct ControlnetComponent : public ECS::BaseComponent {
    std::string image_reference; 
    float strength;              
    float apply_start;
    float apply_end;             

    ControlnetComponent(const std::string &imageRef, float str, float start, float end)
        : image_reference(imageRef), strength(str), apply_start(start), apply_end(end) {}
};
}
