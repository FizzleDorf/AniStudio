#pragma once

#include "BaseComponent.hpp"
//#include "AniStudioDefaults.h"

namespace ECS {
struct DiffusionModelComponent : public ECS::BaseComponent {
    std::string model_path = "path/to/model";
    std::string model_name = "path/to/model";
    bool is_loaded = false;
};
}
