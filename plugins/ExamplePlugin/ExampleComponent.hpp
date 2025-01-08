#pragma once
#include "ECS/Base/BaseComponent.hpp"

namespace ExamplePlugin {

struct ExampleComponent : public ECS::BaseComponent {
    ExampleComponent() { compName = "ExampleComponent"; }

    float value = 0.0f;
    std::string text = "Example Text";

    // Serialization
    nlohmann::json Serialize() const override {
        auto j = BaseComponent::Serialize();
        j["value"] = value;
        j["text"] = text;
        return j;
    }

    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        if (j.contains("value"))
            value = j["value"];
        if (j.contains("text"))
            text = j["text"];
    }
};

} // namespace ExamplePlugin