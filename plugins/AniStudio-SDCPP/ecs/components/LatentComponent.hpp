#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"
#include "DiffusionOptions.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace ECS {
    struct LatentComponent : public ECS::BaseComponent {
        int latentWidth = 512;
        int latentHeight = 512;
        enum rng_type_t current_rng_type = STD_DEFAULT_RNG;
        enum rng_type_t sampler_rng_type = STD_DEFAULT_RNG;
        bool circular_x = false;
        bool circular_y = false;

        LatentComponent() {
            compName = "Latent";
            compCategory = "Sampling";

            schema = {
                {"title", "Latent Settings"},
                {"type", "object"},
                {"propertyOrder", {"latentWidth", "latentHeight", "current_rng_type", "sampler_rng_type", "circular_x", "circular_y"}},
                {"properties", {
                    {"latentWidth", {
                        {"type", "integer"},
                        {"title", "Width"},
                        {"description", "Width of the latent space (must be multiple of 64)."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 64},
                            {"step_fast", 64},
                            {"min", 64},
                            {"max", 2048}
                        }}
                    }},
                    {"latentHeight", {
                        {"type", "integer"},
                        {"title", "Height"},
                        {"description", "Height of the latent space (must be multiple of 64)."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 64},
                            {"step_fast", 64},
                            {"min", 64},
                            {"max", 2048}
                        }}
                    }},
                    {"current_rng_type", {
                        {"type", "integer"},
                        {"title", "RNG Type (initial noise)"},
                        {"description", "Random number generator for initial latent noise."},
                        {"ui:widget", "combo"},
                        {"items", type_rng_items},
                        {"itemCount", type_rng_item_count}
                    }},
                    {"sampler_rng_type", {
                        {"type", "integer"},
                        {"title", "Sampler RNG Type"},
                        {"description", "Random number generator used during sampling."},
                        {"ui:widget", "combo"},
                        {"items", type_rng_items},
                        {"itemCount", type_rng_item_count}
                    }},
                    {"circular_x", {
                        {"type", "boolean"},
                        {"title", "Circular X"},
                        {"description", "Enable circular padding in X direction (for seamless tiling)."},
                        {"default", false}
                    }},
                    {"circular_y", {
                        {"type", "boolean"},
                        {"title", "Circular Y"},
                        {"description", "Enable circular padding in Y direction (for seamless tiling)."},
                        {"default", false}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"latentWidth", &latentWidth},
                {"latentHeight", &latentHeight},
                {"current_rng_type", reinterpret_cast<int*>(&current_rng_type)},
                {"sampler_rng_type", reinterpret_cast<int*>(&sampler_rng_type)},
                {"circular_x", &circular_x},
                {"circular_y", &circular_y}
            };
        }

        LatentComponent& operator=(const LatentComponent& other) {
            if (this != &other) {
                latentWidth = other.latentWidth;
                latentHeight = other.latentHeight;
                current_rng_type = other.current_rng_type;
                sampler_rng_type = other.sampler_rng_type;
                circular_x = other.circular_x;
                circular_y = other.circular_y;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"latentWidth", latentWidth},
                {"latentHeight", latentHeight},
                {"current_rng_type", static_cast<int>(current_rng_type)},
                {"sampler_rng_type", static_cast<int>(sampler_rng_type)},
                {"circular_x", circular_x},
                {"circular_y", circular_y}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);

            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else if (j.is_object() && j.size() == 1) {
                componentData = j.begin().value();
            }
            else {
                componentData = j;
            }

            if (componentData.contains("latentWidth"))
                latentWidth = componentData["latentWidth"];
            if (componentData.contains("latentHeight"))
                latentHeight = componentData["latentHeight"];
            if (componentData.contains("current_rng_type"))
                current_rng_type = static_cast<rng_type_t>(componentData["current_rng_type"].get<int>());
            if (componentData.contains("sampler_rng_type"))
                sampler_rng_type = static_cast<rng_type_t>(componentData["sampler_rng_type"].get<int>());
            if (componentData.contains("circular_x"))
                circular_x = componentData["circular_x"];
            if (componentData.contains("circular_y"))
                circular_y = componentData["circular_y"];
        }
    };
}