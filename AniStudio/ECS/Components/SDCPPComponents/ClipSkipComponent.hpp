#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

    struct ClipSkipComponent : public ECS::BaseComponent {
        ClipSkipComponent() { compName = "ClipSkip"; }
        float clipSkip = 2.0f;

        ClipSkipComponent& operator=(const ClipSkipComponent& other) {
            if (this != &other) {
                clipSkip = other.clipSkip;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"clipSkip", clipSkip},
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

            if (componentData.contains("clipSkip"))
                clipSkip = componentData["clipSkip"];
        }
    };

} // namespace ECS
