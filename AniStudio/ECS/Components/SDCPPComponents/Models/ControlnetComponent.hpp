#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "stb_image.h"
#include <string>

namespace ECS {
struct ControlnetComponent : public ECS::BaseComponent {
    std::string controlName = "";
    std::string controlPath = "";
    // std::string image_reference; 
    float cnStrength = 1.0f;            
    float applyStart = 0.0f;
    float applyEnd = 1.0f;             
};
}
