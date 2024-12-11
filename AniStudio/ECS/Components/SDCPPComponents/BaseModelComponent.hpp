#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct BaseModelComponent : public BaseComponent {
    std::string modelPath = "";
    std::string modelName = "model.gguf";
    bool isModelLoaded = false;

    // Serialize to JSON
    nlohmann::json Serialize() const override {
        nlohmann::json j = BaseComponent::Serialize();
        j["modelPath"] = modelPath;
        j["modelName"] = modelName;
        return j;
    }

    // Deserialize from JSON
    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        if (j.contains("modelPath"))
            modelPath = j["modelPath"];
        if (j.contains("controlPath"))
            modelName = j["modelName"];
    }
};
} // namespace ECS
