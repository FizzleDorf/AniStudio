#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct CLipLComponent : public ECS::BaseComponent {
    
    std::string encoderPath = "";
    std::string encoderName = "model.gguf";
    bool isEncoderLoaded = false;

    CLipLComponent &operator=(const CLipLComponent &other) { 
        if (this != &other) {
            encoderPath = other.encoderPath;
            encoderName = other.encoderName;
            isEncoderLoaded = other.isEncoderLoaded;
        }
        return *this;
    }
};
}