#pragma once
#include "BaseComponent.hpp"
namespace ECS {
struct LayerentSkipComponent : public ECS::BaseComponent {
    int *skip_layers = 0;
    size_t skip_layers_count = 0;
    float slg_scale = 0.0f;
    float skip_layer_start = 0.0f;
    float skip_layer_end = 1.0f;

    LayerentSkipComponent &operator=(const LayerentSkipComponent &other) {
        if (this != &other) { // Self-assignment check
            skip_layers = other.skip_layers;
            skip_layers_count = other.skip_layers_count;
            slg_scale = other.slg_scale;
            skip_layer_start = other.skip_layer_start;
            skip_layer_end = other.skip_layer_end;
        }
        return *this;
    }
};
} // namespace ECS