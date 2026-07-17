#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>
#include <filesystem>
#include <unordered_map>
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"

namespace ECS {

    // TODO: Probably a better way of doing this
    inline sd_vae_format_t vae_format_index_to_enum(int index) {
        switch (index) {
        case 0: return SD_VAE_FORMAT_AUTO;
        case 1: return SD_VAE_FORMAT_FLUX;
        case 2: return SD_VAE_FORMAT_SD3;
        case 3: return SD_VAE_FORMAT_FLUX2;
        default: return SD_VAE_FORMAT_AUTO;
        }
    }

    inline int vae_format_enum_to_index(sd_vae_format_t fmt) {
        switch (fmt) {
        case SD_VAE_FORMAT_AUTO:  return 0;
        case SD_VAE_FORMAT_FLUX:  return 1;
        case SD_VAE_FORMAT_SD3:   return 2;
        case SD_VAE_FORMAT_FLUX2: return 3;
        default: return 0;
        }
    }

    struct VaeComponent : public BaseModelComponent {
        VaeComponent() {
            compName = "Vae";

            schema = {
                {"title", "VAE Settings"},
                {"type", "object"},
                {"propertyOrder", {"modelPath", "isTiled", "tile_size_x", "tile_size_y", "target_overlap", "rel_size_x", "rel_size_y", "keep_vae_on_cpu", "vae_decode_only", "vae_format"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "VAE"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt"},
                            {"filterName", "VAE Models"},
                            {"dialogDefaultPath", "Vae"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for VAE model files (.safetensors, .ckpt, .pt)"}
                        }}
                    }},
                    {"isTiled", {
                        {"type", "boolean"},
                        {"title", "Tiled VAE"},
                        {"description", "Enable tiled VAE processing to reduce memory usage for large images. Trades speed for memory efficiency."},
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
                        {"type", "integer"},
                        {"title", "VAE Format"},
                        {"description", "Format of the VAE model (auto, flux, sd3, flux2)."},
                        {"ui:widget", "combo"},
                        {"items", vae_format_items},
                        {"itemCount", vae_format_item_count}
                    }}
                }}
            };
        }

        int vae_format = 0;
        int tile_size_x, tile_size_y = 64;
        float target_overlap = 0;
        float rel_size_x, rel_size_y = 64;
        bool keep_vae_on_cpu = false;
        bool vae_decode_only = false;
        bool isTiled = false;

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"modelPath", &modelPath},
                {"modelName", &modelName},
                {"isTiled", &isTiled},
                {"tile_size_x", &tile_size_x},
                {"tile_size_y", &tile_size_y},
                {"target_overlap", &target_overlap},
                {"rel_size_x", &rel_size_x},
                {"rel_size_y", &rel_size_y},
                {"keep_vae_on_cpu", &keep_vae_on_cpu},
                {"vae_decode_only", &vae_decode_only},
                {"vae_format", &vae_format}
            };
        }

        std::filesystem::path GetDefaultDirectory() const override {
            return Utils::FilePathService::GetPath("Vae");
        }

        VaeComponent& operator=(const VaeComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
                isTiled = other.isTiled;
                tile_size_x = other.tile_size_x;
                tile_size_y = other.tile_size_y;
                target_overlap = other.target_overlap;
                rel_size_x = other.rel_size_x;
                rel_size_y = other.rel_size_y;
                keep_vae_on_cpu = other.keep_vae_on_cpu;
                vae_decode_only = other.vae_decode_only;
                vae_format = other.vae_format;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            // Store the actual enum value for backward compatibility
            return { {compName, {
                {"modelName", modelName},
                {"modelPath", modelPath},
                {"isTiled", isTiled},
                {"tile_size_x", tile_size_x},
                {"tile_size_y", tile_size_y},
                {"target_overlap", target_overlap},
                {"rel_size_x", rel_size_x},
                {"rel_size_y", rel_size_y},
                {"keep_vae_on_cpu", keep_vae_on_cpu},
                {"vae_decode_only", vae_decode_only},
                {"vae_format", static_cast<int>(vae_format_index_to_enum(vae_format))}
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

            if (componentData.contains("isTiled"))
                isTiled = componentData["isTiled"].get<bool>();
            if (componentData.contains("tile_size_x"))
                tile_size_x = componentData["tile_size_x"].get<int>();
            if (componentData.contains("tile_size_y"))
                tile_size_y = componentData["tile_size_y"].get<int>();
            if (componentData.contains("target_overlap"))
                target_overlap = componentData["target_overlap"].get<float>();
            if (componentData.contains("rel_size_x"))
                rel_size_x = componentData["rel_size_x"].get<float>();
            if (componentData.contains("rel_size_y"))
                rel_size_y = componentData["rel_size_y"].get<float>();
            if (componentData.contains("keep_vae_on_cpu"))
                keep_vae_on_cpu = componentData["keep_vae_on_cpu"].get<bool>();
            if (componentData.contains("vae_decode_only"))
                vae_decode_only = componentData["vae_decode_only"].get<bool>();
            if (componentData.contains("vae_format")) {
                int enum_val = componentData["vae_format"].get<int>();
                vae_format = vae_format_enum_to_index(static_cast<sd_vae_format_t>(enum_val));
            }
        }

        sd_vae_format_t get_vae_format_enum() const {
            return vae_format_index_to_enum(vae_format);
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
                            {"dialogDefaultPath", "Vae"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for TAESD fast VAE files"}
                        }}
                    }}
                }}
            };
        }

        const char * GetDefaultDirectory() const override {
            return "Vae";
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

} // namespace ECS