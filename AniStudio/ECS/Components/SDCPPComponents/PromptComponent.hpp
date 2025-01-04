#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

struct PromptComponent : public BaseComponent {
    PromptComponent() { compName = "PromptComponent"; }
    std::string posPrompt = "";
    std::string negPrompt = "";
    char PosBuffer[9999] = "Positive";
    char NegBuffer[9999] = "Negative";

    PromptComponent &operator=(const PromptComponent &other) {
        if (this != &other) {
            posPrompt = other.posPrompt;
            negPrompt = other.negPrompt;
        }
        return *this;
    }

    nlohmann::json Serialize() const { return {{compName,
                 {
                  {"posPrompt", posPrompt},
                  {"negPrompt", negPrompt}}}};
    }

    static PromptComponent Deserialize(const nlohmann::json &json) {
        if (!json.contains("PromptComponent")) {
            throw std::runtime_error("Invalid JSON structure for deserialization");
        }

        const auto &componentData = json.at("PromptComponent");
        PromptComponent component;

        if (componentData.contains("posPrompt")) {
            component.posPrompt = componentData.at("posPrompt").get<std::string>();
        }
        if (componentData.contains("negPrompt")) {
            component.negPrompt = componentData.at("negPrompt").get<std::string>();
        }

        return component;
    }
};
}
