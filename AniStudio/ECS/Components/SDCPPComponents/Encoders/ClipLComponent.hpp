#pragma once

#include "BaseComponent.hpp"

namespace ECS {
struct CLipLComponent : public ECS::CLipLComponent {
    std::string model_path = "path/to/model";
    std::string model_name = "model.gguf";
    bool is_loaded = false;
};
}