#pragma once

#include "BaseComponent.hpp"
#include "ModelComponents.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>

namespace ECS {

    struct HighNoiseSamplerComponent : public BaseComponent {
        std::string high_noise_sample_method = "EULER";
        std::string high_noise_scheduler_method = "DISCRETE";
        int high_noise_steps = 8;
        float high_noise_cfg = 3.5f;
        float high_noise_eta = 0.0f;

        HighNoiseSamplerComponent() = default;

        const char* GetCompName() const override { return "HighNoiseSampler"; }
        const char* GetCompCategory() const override { return "Sampling"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
            {"title", "High Noise Sampler Settings"},
            {"type", "object"},
            {"propertyOrder", {"high_noise_sample_method", "high_noise_scheduler_method",
                              "high_noise_cfg", "high_noise_steps", "high_noise_eta"}},
            {"properties", {
                {"high_noise_sample_method", {
                    {"type", "string"},
                    {"title", "High Noise Sampler"},
                    {"description", "Sampling method for high noise model in Wan 2.2 dual-model setup."},
                    {"ui:widget", "combo"},
                    {"items", get_sample_method_names()},
                    {"itemCount", static_cast<int>(get_sample_method_names().size())}
                }},
                {"high_noise_scheduler_method", {
                    {"type", "string"},
                    {"title", "High Noise Scheduler"},
                    {"description", "Scheduler for high noise model sampling."},
                    {"ui:widget", "combo"},
                    {"items", get_scheduler_names()},
                    {"itemCount", static_cast<int>(get_scheduler_names().size())}
                }},
                {"high_noise_cfg", {
                    {"type", "number"},
                    {"title", "High Noise CFG"},
                    {"description", "CFG scale for high noise model. Usually same as main CFG."},
                    {"ui:widget", "input_float"},
                    {"ui:options", {
                        {"step", 0.5f},
                        {"step_fast", 1.0f},
                        {"format", "%.2f"},
                        {"min", 1.0f},
                        {"max", 20.0f}
                    }}
                }},
                {"high_noise_steps", {
                    {"type", "integer"},
                    {"title", "High Noise Steps"},
                    {"description", "Number of steps for high noise model. Usually fewer than main steps."},
                    {"ui:widget", "input_int"},
                    {"ui:options", {
                        {"step", 1},
                        {"step_fast", 2},
                        {"min", 1},
                        {"max", 50}
                    }}
                }},
                {"high_noise_eta", {
                    {"type", "number"},
                    {"title", "High Noise ETA"},
                    {"description", "Eta parameter for high noise sampler. Controls noise addition during sampling."},
                    {"ui:widget", "input_float"},
                    {"ui:options", {
                        {"step", 0.05f},
                        {"step_fast", 0.1f},
                        {"format", "%.2f"},
                        {"min", 0.0f},
                        {"max", 1.0f}
                    }}
                }}
            }}
            };
            return j;
        }

        HighNoiseSamplerComponent(const HighNoiseSamplerComponent& other) : BaseComponent(other) {
            high_noise_sample_method = other.high_noise_sample_method;
            high_noise_scheduler_method = other.high_noise_scheduler_method;
            high_noise_steps = other.high_noise_steps;
            high_noise_cfg = other.high_noise_cfg;
            high_noise_eta = other.high_noise_eta;
        }

        HighNoiseSamplerComponent& operator=(const HighNoiseSamplerComponent& other) {
            if (this != &other) {
                high_noise_sample_method = other.high_noise_sample_method;
                high_noise_scheduler_method = other.high_noise_scheduler_method;
                high_noise_cfg = other.high_noise_cfg;
                high_noise_steps = other.high_noise_steps;
                high_noise_eta = other.high_noise_eta;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"high_noise_sample_method", &high_noise_sample_method},
                {"high_noise_scheduler_method", &high_noise_scheduler_method},
                {"high_noise_cfg", &high_noise_cfg},
                {"high_noise_steps", &high_noise_steps},
                {"high_noise_eta", &high_noise_eta}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"high_noise_sample_method", high_noise_sample_method},
                {"high_noise_scheduler_method", high_noise_scheduler_method},
                {"high_noise_cfg", high_noise_cfg},
                {"high_noise_steps", high_noise_steps},
                {"high_noise_eta", high_noise_eta}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();

            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == key) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("high_noise_cfg")) high_noise_cfg = componentData["high_noise_cfg"];
            if (componentData.contains("high_noise_steps")) high_noise_steps = componentData["high_noise_steps"];
            if (componentData.contains("high_noise_eta")) high_noise_eta = componentData["high_noise_eta"];

            if (componentData.contains("high_noise_sample_method")) {
                const auto& val = componentData["high_noise_sample_method"];
                if (val.is_string()) high_noise_sample_method = val.get<std::string>();
            }
            if (componentData.contains("high_noise_scheduler_method")) {
                const auto& val = componentData["high_noise_scheduler_method"];
                if (val.is_string()) high_noise_scheduler_method = val.get<std::string>();
            }
        }
    };

} // namespace ECS