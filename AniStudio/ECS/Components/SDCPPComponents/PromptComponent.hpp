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
            strncpy(PosBuffer, posPrompt.c_str(), sizeof(PosBuffer) - 1);
            strncpy(NegBuffer, negPrompt.c_str(), sizeof(NegBuffer) - 1);
            PosBuffer[sizeof(PosBuffer) - 1] = '\0'; // Ensure null termination
            NegBuffer[sizeof(NegBuffer) - 1] = '\0'; // Ensure null termination
        }
        return *this;
    }

    nlohmann::json Serialize() const { return {{compName,
                 {
                  {"posPrompt", posPrompt},
                  {"negPrompt", negPrompt}}}};
    }

    void Deserialize(const nlohmann::json &json) {
        if (!json.contains("PromptComponent")) {
            throw std::runtime_error("Invalid JSON structure for deserialization");
        }
        const auto &componentData = json.at("PromptComponent");
        if (componentData.contains("posPrompt")) {
            posPrompt = componentData.at("posPrompt").get<std::string>();
            strncpy(PosBuffer, posPrompt.c_str(), sizeof(PosBuffer) - 1);
            PosBuffer[sizeof(PosBuffer) - 1] = '\0'; // Ensure null termination
        }
        if (componentData.contains("negPrompt")) {
            negPrompt = componentData.at("negPrompt").get<std::string>();
            strncpy(NegBuffer, negPrompt.c_str(), sizeof(NegBuffer) - 1);
            NegBuffer[sizeof(NegBuffer) - 1] = '\0'; // Ensure null termination
        }
    }
};
}
