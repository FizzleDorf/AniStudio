#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

struct EncoderComponent : public ECS::BaseComponent {
    std::string encoderPath = "";
    std::string encoderName = "model.gguf";
    bool isEncoderLoaded = false;

    EncoderComponent &operator=(const EncoderComponent &other) {
        if (this != &other) { // Self-assignment check
            encoderPath = other.encoderPath;
            encoderName = other.encoderName;
            isEncoderLoaded = other.isEncoderLoaded;
        }
        return *this;
    }

    EncoderComponent() = default;
};

struct CLipGComponent : public ECS::EncoderComponent {};
struct CLipLComponent : public ECS::EncoderComponent {};
struct T5XXLComponent : public ECS::EncoderComponent {};
//struct T5XLComponent : public ECS::BaseComponent {};
} // namespace ECS