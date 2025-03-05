#pragma once

// TODO: reduce to a single input when execution pipeline is completed

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

struct PromptComponent : public BaseComponent {
    PromptComponent() { compName = "Prompt"; }
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

    void Deserialize(const nlohmann::json& j) override {
        nlohmann::json componentData;

        if (j.contains(compName)) {
            componentData = j.at(compName);
        }
        else {
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (it.key() == compName) {
                    componentData = it.value();
                    break;
                }
            }
            if (componentData.empty()) {
                componentData = j;
            }
        }

        if (componentData.contains("posPrompt")) {
            posPrompt = componentData["posPrompt"].get<std::string>();
            strncpy(PosBuffer, posPrompt.c_str(), sizeof(PosBuffer) - 1);
            PosBuffer[sizeof(PosBuffer) - 1] = '\0'; // Ensure null termination
        }

        if (componentData.contains("negPrompt")) {
            negPrompt = componentData["negPrompt"].get<std::string>();
            strncpy(NegBuffer, negPrompt.c_str(), sizeof(NegBuffer) - 1);
            NegBuffer[sizeof(NegBuffer) - 1] = '\0'; // Ensure null termination
        }
    }
};
}
