#pragma once

#include "BaseComponent.hpp"

namespace ECS {

    struct ChromaComponent : public BaseComponent {
        bool use_dit_mask = false;
        bool use_t5_mask = false;
        int t5_mask_pad = 0;
        bool enable_chroma_mode = false;
        std::string chroma_model_type = "standard";

        ChromaComponent() {
            compName = "Chroma";
            compCategory = "Advanced";

            schema = {
                {"title", "Chroma Settings"},
                {"type", "object"},
                {"properties", {
                    {"use_dit_mask", {
                        {"type", "boolean"},
                        {"title", "Use DiT Mask"},
                        {"description", "Enable DiT masking for Chroma models"},
                        {"default", false}
                    }},
                    {"use_t5_mask", {
                        {"type", "boolean"},
                        {"title", "Use T5 Mask"},
                        {"description", "Enable T5 text encoder masking"},
                        {"default", false}
                    }},
                    {"t5_mask_pad", {
                        {"type", "integer"},
                        {"title", "T5 Mask Padding"},
                        {"description", "Number of padding tokens to unmask in T5 encoder (0 = auto)"},
                        {"minimum", 0},
                        {"maximum", 100},
                        {"default", 0}
                    }},
                    {"enable_chroma_mode", {
                        {"type", "boolean"},
                        {"title", "Enable Chroma Mode"},
                        {"description", "Enable Chroma-specific optimizations"},
                        {"default", false}
                    }},
                    {"chroma_model_type", {
                        {"type", "string"},
                        {"title", "Chroma Model Type"},
                        {"description", "Type of Chroma model being used"},
                        {"ui:widget", "combo"},
                        {"items", {"standard", "flux_schnell", "experimental"}},
                        {"default", "standard"}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"use_dit_mask", &use_dit_mask},
                {"use_t5_mask", &use_t5_mask},
                {"t5_mask_pad", &t5_mask_pad},
                {"enable_chroma_mode", &enable_chroma_mode},
                {"chroma_model_type", &chroma_model_type}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j = BaseComponent::Serialize();
            j[compName] = {
                {"use_dit_mask", use_dit_mask},
                {"use_t5_mask", use_t5_mask},
                {"t5_mask_pad", t5_mask_pad},
                {"enable_chroma_mode", enable_chroma_mode},
                {"chroma_model_type", chroma_model_type}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            if (j.contains(compName)) {
                auto comp = j[compName];
                if (comp.contains("use_dit_mask")) use_dit_mask = comp["use_dit_mask"];
                if (comp.contains("use_t5_mask")) use_t5_mask = comp["use_t5_mask"];
                if (comp.contains("t5_mask_pad")) t5_mask_pad = comp["t5_mask_pad"];
                if (comp.contains("enable_chroma_mode")) enable_chroma_mode = comp["enable_chroma_mode"];
                if (comp.contains("chroma_model_type")) chroma_model_type = comp["chroma_model_type"];
            }
        }

        std::string get_model_args() const {
            std::string args;
            args += "chroma_use_dit_mask=";
            args += use_dit_mask ? "true" : "false";
            args += ",chroma_use_t5_mask=";
            args += use_t5_mask ? "true" : "false";
            if (t5_mask_pad > 0) {
                args += ",chroma_t5_mask_pad=" + std::to_string(t5_mask_pad);
            }
            return args;
        }
    };

    struct StackedIdEmbedComponent : public BaseComponent {
        std::string modelName = "";
        std::string modelPath = "";
        bool enabled = false;
        float strength = 1.0f;

        StackedIdEmbedComponent() {
            compName = "StackedIdEmbed";
            compCategory = "Model";

            schema = {
                {"title", "Stacked ID Embedding"},
                {"type", "object"},
                {"properties", {
                    {"modelName", {
                        {"type", "string"},
                        {"title", "Model Name"},
                        {"description", "Name of the stacked ID embedding model"},
                        {"default", ""}
                    }},
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "Model Path"},
                        {"description", "Full path to the stacked ID embedding model file"},
                        {"default", ""}
                    }},
                    {"enabled", {
                        {"type", "boolean"},
                        {"title", "Enabled"},
                        {"description", "Enable stacked ID embedding processing"},
                        {"default", false}
                    }},
                    {"strength", {
                        {"type", "number"},
                        {"title", "Strength"},
                        {"description", "Strength of the ID embedding effect"},
                        {"minimum", 0.0},
                        {"maximum", 2.0},
                        {"default", 1.0}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"modelName", &modelName},
                {"modelPath", &modelPath},
                {"enabled", &enabled},
                {"strength", &strength}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j = BaseComponent::Serialize();
            j[compName] = {
                {"modelName", modelName},
                {"modelPath", modelPath},
                {"enabled", enabled},
                {"strength", strength}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            if (j.contains(compName)) {
                auto comp = j[compName];
                if (comp.contains("modelName")) modelName = comp["modelName"];
                if (comp.contains("modelPath")) modelPath = comp["modelPath"];
                if (comp.contains("enabled")) enabled = comp["enabled"];
                if (comp.contains("strength")) strength = comp["strength"];
            }
        }
    };

} // namespace ECS