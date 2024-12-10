#pragma once

#include "BaseComponent.hpp"
#include "nlohmann/json.hpp"
#include <string>

namespace ECS {
struct ControlnetComponent : public ECS::BaseComponent {
    ControlnetComponent() { compName = "Controlnet_Component"; }

    std::string controlName = "";
    std::string controlPath = "";
    float cnStrength = 1.0f;
    float applyStart = 0.0f;
    float applyEnd = 1.0f;

    // Operator = Overload for copying a component
     ControlnetComponent &operator=(const ControlnetComponent &other) {
        if (this != &other) {
            controlName = other.controlName;
            controlPath = other.controlPath;
            cnStrength = other.cnStrength;
            applyStart = other.applyStart;
            applyEnd = other.applyEnd;
        }
        return *this;
    }

    // Serialize to JSON
    nlohmann::json Serialize() const override {
        nlohmann::json j = BaseComponent::Serialize();
        j["controlName"] = controlName;
        j["controlPath"] = controlPath;
        j["cnStrength"] = cnStrength;
        j["applyStart"] = applyStart;
        j["applyEnd"] = applyEnd;
        return j;
    }

    // Deserialize from JSON
    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        if (j.contains("controlName"))
            controlName = j["controlName"];
        if (j.contains("controlPath"))
            controlPath = j["controlPath"];
        if (j.contains("cnStrength"))
            cnStrength = j["cnStrength"];
        if (j.contains("applyStart"))
            applyStart = j["applyStart"];
        if (j.contains("applyEnd"))
            applyEnd = j["applyEnd"];
    }
};
} // namespace ECS
