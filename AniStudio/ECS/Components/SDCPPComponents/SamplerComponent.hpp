#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {
struct SamplerComponent : public ECS::BaseComponent {
    std::string sampler;    
    std::string scheduler;  
    int steps;              
    float denoise;          

    SamplerComponent(float cfg_value = 7.0f, const std::string &sampler_name = "",
                     const std::string &scheduler_name = "", int num_steps = 50, float denoise_value = 0.5f)
        : sampler(sampler_name), scheduler(scheduler_name), steps(num_steps), denoise(denoise_value){}
};
}
