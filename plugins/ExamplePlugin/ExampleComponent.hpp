#pragma once
#include "ECS.h"
#include "../../external/nlohmann_json/nlohmann/json.hpp"
#include "DLLDefines.hpp"

namespace ECS {

struct ANI_API ExampleComponent : public BaseComponent {
    ExampleComponent() { compName = "ExampleComponent"; }

    int count = 0;
    float updateRate = 1.0f;
    bool autoIncrement = false;
    float timeSinceLastUpdate = 0.0f;

    nlohmann::json Serialize() const override {
        auto j = BaseComponent::Serialize();
        j[compName] = {{"count", count}, {"updateRate", updateRate}, {"autoIncrement", autoIncrement}};
        return j;
    }

    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        if (j.contains(compName)) {
            const auto &data = j[compName];
            if (data.contains("count"))
                count = data["count"];
            if (data.contains("updateRate"))
                updateRate = data["updateRate"];
            if (data.contains("autoIncrement"))
                autoIncrement = data["autoIncrement"];
        }
    }
};

} // namespace CounterPlugin