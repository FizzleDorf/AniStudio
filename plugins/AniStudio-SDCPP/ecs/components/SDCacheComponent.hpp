// SDCacheComponent.hpp
#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>

namespace ECS {

    struct EasyCacheComponent : public BaseComponent {
        std::string mode = "DISABLED";
        float reuse_threshold = 1.0f;
        float start_percent = 0.15f;
        float end_percent = 0.95f;
        float error_decay_rate = 1.0f;
        bool use_relative_threshold = true;
        bool reset_error_on_compute = true;
        int Fn_compute_blocks = 8;
        int Bn_compute_blocks = 0;
        float residual_diff_threshold = 0.08f;
        int max_warmup_steps = 8;
        int max_cached_steps = -1;
        int max_continuous_cached_steps = -1;
        int taylorseer_n_derivatives = 1;
        int taylorseer_skip_interval = 1;
        std::string scm_mask;
        bool scm_policy_dynamic = true;
        float spectrum_w = 0.40f;
        int spectrum_m = 3;
        float spectrum_lam = 1.0f;
        int spectrum_window_size = 2;
        float spectrum_flex_window = 0.50f;
        int spectrum_warmup_steps = 4;
        float spectrum_stop_percent = 0.9f;

        EasyCacheComponent() {
            compName = "EasyCache";
            compCategory = "Performance";

            schema = {
                {"title", "EasyCache Settings"},
                {"type", "object"},
                {"description", "Cache management for faster generation (set mode to EASYCACHE to enable)."},
                {"properties", {
                    {"mode", {
                        {"type", "integer"},
                        {"title", "Cache Mode"},
                        {"description", "Cache mode selection. EasyCache for DiT models, UCache/Spectrum for UNet models."},
                        {"ui:widget", "combo"},
                        {"items", get_cache_mode_names()},
                        {"itemCount", static_cast<int>(get_cache_mode_names().size())}
                    }},
                    {"reuse_threshold", {
                        {"type", "number"},
                        {"title", "Reuse Threshold"},
                        {"description", "Threshold for reusing cached values (0.0-1.0)."},
                        {"default", 1.0f},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.05f},
                            {"step_fast", 0.1f},
                            {"min", 0.0f},
                            {"max", 1.0f}
                        }}
                    }},
                    {"start_percent", {
                        {"type", "number"},
                        {"title", "Start Percentage"},
                        {"description", "When to start caching (0.0-1.0)."},
                        {"default", 0.15f},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.05f},
                            {"step_fast", 0.1f},
                            {"min", 0.0f},
                            {"max", 1.0f}
                        }}
                    }},
                    {"end_percent", {
                        {"type", "number"},
                        {"title", "End Percentage"},
                        {"description", "When to stop caching (0.0-1.0)."},
                        {"default", 0.95f},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.05f},
                            {"step_fast", 0.1f},
                            {"min", 0.0f},
                            {"max", 1.0f}
                        }}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"mode", &mode},
                {"reuse_threshold", &reuse_threshold},
                {"start_percent", &start_percent},
                {"end_percent", &end_percent}
            };
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"mode", mode},
                {"reuse_threshold", reuse_threshold},
                {"start_percent", start_percent},
                {"end_percent", end_percent},
                {"error_decay_rate", error_decay_rate},
                {"use_relative_threshold", use_relative_threshold},
                {"reset_error_on_compute", reset_error_on_compute},
                {"Fn_compute_blocks", Fn_compute_blocks},
                {"Bn_compute_blocks", Bn_compute_blocks},
                {"residual_diff_threshold", residual_diff_threshold},
                {"max_warmup_steps", max_warmup_steps},
                {"max_cached_steps", max_cached_steps},
                {"max_continuous_cached_steps", max_continuous_cached_steps},
                {"taylorseer_n_derivatives", taylorseer_n_derivatives},
                {"taylorseer_skip_interval", taylorseer_skip_interval},
                {"scm_mask", scm_mask},
                {"scm_policy_dynamic", scm_policy_dynamic},
                {"spectrum_w", spectrum_w},
                {"spectrum_m", spectrum_m},
                {"spectrum_lam", spectrum_lam},
                {"spectrum_window_size", spectrum_window_size},
                {"spectrum_flex_window", spectrum_flex_window},
                {"spectrum_warmup_steps", spectrum_warmup_steps},
                {"spectrum_stop_percent", spectrum_stop_percent}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);

            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                componentData = j;
            }

            if (componentData.contains("mode")) {
                const auto& val = componentData["mode"];
                if (val.is_string()) mode = val.get<std::string>();
            }
            if (componentData.contains("reuse_threshold")) reuse_threshold = componentData["reuse_threshold"].get<float>();
            if (componentData.contains("start_percent")) start_percent = componentData["start_percent"].get<float>();
            if (componentData.contains("end_percent")) end_percent = componentData["end_percent"].get<float>();
            if (componentData.contains("error_decay_rate")) error_decay_rate = componentData["error_decay_rate"].get<float>();
            if (componentData.contains("use_relative_threshold")) use_relative_threshold = componentData["use_relative_threshold"].get<bool>();
            if (componentData.contains("reset_error_on_compute")) reset_error_on_compute = componentData["reset_error_on_compute"].get<bool>();
            if (componentData.contains("Fn_compute_blocks")) Fn_compute_blocks = componentData["Fn_compute_blocks"].get<int>();
            if (componentData.contains("Bn_compute_blocks")) Bn_compute_blocks = componentData["Bn_compute_blocks"].get<int>();
            if (componentData.contains("residual_diff_threshold")) residual_diff_threshold = componentData["residual_diff_threshold"].get<float>();
            if (componentData.contains("max_warmup_steps")) max_warmup_steps = componentData["max_warmup_steps"].get<int>();
            if (componentData.contains("max_cached_steps")) max_cached_steps = componentData["max_cached_steps"].get<int>();
            if (componentData.contains("max_continuous_cached_steps")) max_continuous_cached_steps = componentData["max_continuous_cached_steps"].get<int>();
            if (componentData.contains("taylorseer_n_derivatives")) taylorseer_n_derivatives = componentData["taylorseer_n_derivatives"].get<int>();
            if (componentData.contains("taylorseer_skip_interval")) taylorseer_skip_interval = componentData["taylorseer_skip_interval"].get<int>();
            if (componentData.contains("scm_mask")) scm_mask = componentData["scm_mask"].get<std::string>();
            if (componentData.contains("scm_policy_dynamic")) scm_policy_dynamic = componentData["scm_policy_dynamic"].get<bool>();
            if (componentData.contains("spectrum_w")) spectrum_w = componentData["spectrum_w"].get<float>();
            if (componentData.contains("spectrum_m")) spectrum_m = componentData["spectrum_m"].get<int>();
            if (componentData.contains("spectrum_lam")) spectrum_lam = componentData["spectrum_lam"].get<float>();
            if (componentData.contains("spectrum_window_size")) spectrum_window_size = componentData["spectrum_window_size"].get<int>();
            if (componentData.contains("spectrum_flex_window")) spectrum_flex_window = componentData["spectrum_flex_window"].get<float>();
            if (componentData.contains("spectrum_warmup_steps")) spectrum_warmup_steps = componentData["spectrum_warmup_steps"].get<int>();
            if (componentData.contains("spectrum_stop_percent")) spectrum_stop_percent = componentData["spectrum_stop_percent"].get<float>();
        }
    };

} // namespace ECS