#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct LatentComponent : public ECS::BaseComponent {
    LatentComponent() { compName = "LatentComponent"; }
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
    void Deserialize(const nlohmann::json &j) {
        if (j.contains(compName)) {
            const auto &obj = j.at("LatentComponent");
            if (obj.contains("latentWidth"))
                latentWidth = obj["latentWidth"];
            if (obj.contains("latentHeight"))
                latentHeight = obj["latentHeight"];
            if (obj.contains("batchSize"))
                batchSize = obj["batchSize"];
        }
    }
};
}
