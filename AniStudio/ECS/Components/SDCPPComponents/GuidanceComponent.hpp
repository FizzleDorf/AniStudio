#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

    struct GuidanceComponent : public ECS::BaseComponent {
        GuidanceComponent() { compName = "Guidance"; }
        float guidance = 2;
        float eta = 0;
        
        GuidanceComponent& operator=(const GuidanceComponent& other) {
            if (this != &other) {
                guidance = other.guidance;
                eta = other.eta;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"guidance", guidance},
                      {"eta", eta},
                      }} };
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

            if (componentData.contains("guidance"))
                guidance = componentData["guidance"];
            if (componentData.contains("eta"))
                eta = componentData["eta"];
        }
    };

} // namespace ECS
