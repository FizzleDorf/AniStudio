#pragma once

#include "BaseModelComponent.hpp"
#include "nlohmann/json.hpp"
#include <string>

namespace ECS {
struct ControlnetComponent : public ECS::BaseModelComponent {
    ControlnetComponent() { compName = "Controlnet_Component"; }

    float cnStrength = 1.0f;
    float applyStart = 0.0f;
    float applyEnd = 1.0f;

    // Operator = Overload for copying a component
     ControlnetComponent &operator=(const ControlnetComponent &other) {
        if (this != &other) {
            modelName = other.modelName;
            modelPath = other.modelPath;
            cnStrength = other.cnStrength;
            applyStart = other.applyStart;
            applyEnd = other.applyEnd;
        }
        return *this;
    }

    // Serialize to JSON
    nlohmann::json Serialize() const override {
        nlohmann::json j = BaseModelComponent::Serialize();
        j["cnStrength"] = cnStrength;
        j["applyStart"] = applyStart;
        j["applyEnd"] = applyEnd;
        return j;
    }

    // Deserialize from JSON
    void Deserialize(const nlohmann::json &j) override {
        BaseModelComponent::Deserialize(j);
        if (j.contains("cnStrength"))
            cnStrength = j["cnStrength"];
        if (j.contains("applyStart"))
            applyStart = j["applyStart"];
        if (j.contains("applyEnd"))
            applyEnd = j["applyEnd"];
    }
};
} // namespace ECS
