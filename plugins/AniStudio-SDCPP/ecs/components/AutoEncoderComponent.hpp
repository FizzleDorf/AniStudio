// AutoEncoderComponent.hpp
#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>
#include <filesystem>
#include <unordered_map>
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"

namespace ECS {

    struct VaeComponent : public BaseModelComponent {
        VaeComponent() {
            compName = "Vae";

            schema = {
                {"title", "VAE Settings"},
                {"type", "object"},
                {"propertyOrder", {"modelPath", "keep_vae_on_cpu", "vae_decode_only", "vae_format"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "VAE"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt"},
                            {"filterName", "VAE Models"},
                            {"dialogDefaultPath", "vae"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for VAE model files (.safetensors, .ckpt, .pt)"}
                        }}
                    }},
                    {"keep_vae_on_cpu", {
                        {"type", "boolean"},
                        {"title", "Keep VAE on CPU"},
                        {"description", "Keep VAE on CPU instead of GPU to save VRAM but significantly reduce performance."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"vae_decode_only", {
                        {"type", "boolean"},
                        {"title", "VAE Decode Only"},
                        {"description", "Only use VAE for decoding (not encoding). Useful for txt2img workflows to save memory."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"vae_format", {
                        {"type", "string"},
                        {"title", "VAE Format"},
                        {"description", "Format of the VAE model (auto, flux, sd3, flux2, wan)."},
                        {"ui:widget", "combo"},
                        {"items", get_vae_format_names()},
                        {"itemCount", static_cast<int>(get_vae_format_names().size())}
                    }}
                }}
            };
        }

        bool keep_vae_on_cpu = false;
        bool vae_decode_only = false;
        std::string vae_format = "AUTO";

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"modelPath", &modelPath},
                {"modelName", &modelName},
                {"keep_vae_on_cpu", &keep_vae_on_cpu},
                {"vae_decode_only", &vae_decode_only},
                {"vae_format", &vae_format}
            };
        }

        VaeComponent& operator=(const VaeComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
                keep_vae_on_cpu = other.keep_vae_on_cpu;
                vae_decode_only = other.vae_decode_only;
                vae_format = other.vae_format;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"modelName", modelName},
                {"modelPath", modelPath},
                {"keep_vae_on_cpu", keep_vae_on_cpu},
                {"vae_decode_only", vae_decode_only},
                {"vae_format", vae_format}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseModelComponent::Deserialize(j);

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

            if (componentData.contains("keep_vae_on_cpu")) keep_vae_on_cpu = componentData["keep_vae_on_cpu"].get<bool>();
            if (componentData.contains("vae_decode_only")) vae_decode_only = componentData["vae_decode_only"].get<bool>();
            if (componentData.contains("vae_format")) {
                const auto& val = componentData["vae_format"];
                if (val.is_string()) {
                    vae_format = val.get<std::string>();
                }
            }
        }

        sd_vae_format_t get_vae_format_enum() const {
            return static_cast<sd_vae_format_t>(vae_format_from_name(vae_format));
        }
    };

    struct VaeTilingComponent : public BaseComponent {
        VaeTilingComponent() {
            compName = "VaeTiling";

            schema = {
                {"title", "VAE Tiling Settings"},
                {"type", "object"},
                {"propertyOrder", {"isTiled", "temporal_tiling", "tile_size_x", "tile_size_y",
                                   "target_overlap", "rel_size_x", "rel_size_y", "extra_tiling_args"}},
                {"properties", {
                    {"isTiled", {
                        {"type", "boolean"},
                        {"title", "Tiled VAE"},
                        {"description", "Enable tiled VAE processing to reduce memory usage for large images. Trades speed for memory efficiency."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"temporal_tiling", {
                        {"type", "boolean"},
                        {"title", "Temporal Tiling"},
                        {"description", "Enable temporal tiling for video processing."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"tile_size_x", {
                        {"type", "integer"},
                        {"title", "Tile Width"},
                        {"description", "Width of each tile in pixels. Smaller values use less memory but are slower."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 8},
                            {"step_fast", 16},
                            {"min", 32},
                            {"max", 512}
                        }}
                    }},
                    {"tile_size_y", {
                        {"type", "integer"},
                        {"title", "Tile Height"},
                        {"description", "Height of each tile in pixels. Smaller values use less memory but are slower."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 8},
                            {"step_fast", 16},
                            {"min", 32},
                            {"max", 512}
                        }}
                    }},
                    {"target_overlap", {
                        {"type", "number"},
                        {"title", "Tile Overlap"},
                        {"description", "Overlap between tiles as a fraction (0.0-1.0). Higher values reduce seams but use more memory."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.05f},
                            {"step_fast", 0.1f},
                            {"min", 0.0f},
                            {"max", 0.5f}
                        }}
                    }},
                    {"rel_size_x", {
                        {"type", "number"},
                        {"title", "Relative Width"},
                        {"description", "Relative width scaling factor for tiles."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 8.0f},
                            {"step_fast", 16.0f},
                            {"min", 8.0f},
                            {"max", 512.0f}
                        }}
                    }},
                    {"rel_size_y", {
                        {"type", "number"},
                        {"title", "Relative Height"},
                        {"description", "Relative height scaling factor for tiles."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 8.0f},
                            {"step_fast", 16.0f},
                            {"min", 8.0f},
                            {"max", 512.0f}
                        }}
                    }},
                    {"extra_tiling_args", {
                        {"type", "string"},
                        {"title", "Extra Tiling Args"},
                        {"description", "Additional tiling arguments as a string."},
                        {"ui:widget", "textarea"}
                    }}
                }}
            };
        }

        bool isTiled = false;
        bool temporal_tiling = false;
        int tile_size_x = 64;
        int tile_size_y = 64;
        float target_overlap = 0.75f;
        float rel_size_x = 64.0f;
        float rel_size_y = 64.0f;
        std::string extra_tiling_args;

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"isTiled", &isTiled},
                {"temporal_tiling", &temporal_tiling},
                {"tile_size_x", &tile_size_x},
                {"tile_size_y", &tile_size_y},
                {"target_overlap", &target_overlap},
                {"rel_size_x", &rel_size_x},
                {"rel_size_y", &rel_size_y},
                {"extra_tiling_args", &extra_tiling_args}
            };
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"isTiled", isTiled},
                {"temporal_tiling", temporal_tiling},
                {"tile_size_x", tile_size_x},
                {"tile_size_y", tile_size_y},
                {"target_overlap", target_overlap},
                {"rel_size_x", rel_size_x},
                {"rel_size_y", rel_size_y},
                {"extra_tiling_args", extra_tiling_args}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json data;
            if (j.contains(compName)) {
                data = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        data = it.value();
                        break;
                    }
                }
                if (data.empty()) {
                    data = j;
                }
            }

            if (data.contains("isTiled")) isTiled = data["isTiled"].get<bool>();
            if (data.contains("temporal_tiling")) temporal_tiling = data["temporal_tiling"].get<bool>();
            if (data.contains("tile_size_x")) tile_size_x = data["tile_size_x"].get<int>();
            if (data.contains("tile_size_y")) tile_size_y = data["tile_size_y"].get<int>();
            if (data.contains("target_overlap")) target_overlap = data["target_overlap"].get<float>();
            if (data.contains("rel_size_x")) rel_size_x = data["rel_size_x"].get<float>();
            if (data.contains("rel_size_y")) rel_size_y = data["rel_size_y"].get<float>();
            if (data.contains("extra_tiling_args")) extra_tiling_args = data["extra_tiling_args"].get<std::string>();
        }
    };

    struct TaesdComponent : public BaseModelComponent {
        TaesdComponent() {
            compName = "Taesd";

            schema = {
                {"title", "TAESD Fast VAE"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "TAESD"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt"},
                            {"filterName", "TAESD Models"},
                            {"dialogDefaultPath", "vae"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for TAESD fast VAE files"}
                        }}
                    }}
                }}
            };
        }

        TaesdComponent& operator=(const TaesdComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

    struct AudioVaeComponent : public BaseModelComponent {
        AudioVaeComponent() {
            compName = "AudioVae";

            schema = {
                {"title", "Audio VAE"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "Audio VAE Model"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt"},
                            {"filterName", "Audio VAE Models"},
                            {"dialogDefaultPath", "vae"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for Audio VAE model files"}
                        }}
                    }}
                }}
            };
        }

        AudioVaeComponent& operator=(const AudioVaeComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

} // namespace ECS