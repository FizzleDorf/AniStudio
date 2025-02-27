#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct LatentComponent : public ECS::BaseComponent {
    LatentComponent() { compName = "Latent"; }
    // unsigned char *latentData = nullptr;
    int latentWidth = 512;
    int latentHeight = 512;
    int batchSize = 1;

    LatentComponent &operator=(const LatentComponent &other) {
        if (this != &other) { // Self-assignment check
            latentWidth = other.latentWidth;
            latentHeight = other.latentHeight;
            batchSize = other.batchSize;
        }
        return *this;
    }

    // Serialize the component to JSON
    nlohmann::json Serialize() const {
        return {{compName,
                 {{"latentWidth", latentWidth}, 
                 {"latentHeight", latentHeight}, 
            {"batchSize", batchSize}}
            }};
    }

    // Deserialize the component from JSON
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

        if (componentData.contains("latentWidth"))
            latentWidth = componentData["latentWidth"];
        if (componentData.contains("latentHeight"))
            latentHeight = componentData["latentHeight"];
        if (componentData.contains("batchSize"))
            batchSize = componentData["batchSize"];
    }
};
}
