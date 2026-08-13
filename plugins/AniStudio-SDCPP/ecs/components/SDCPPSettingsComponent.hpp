// SDCPPSettingsComponent.hpp
#pragma once
#include "BaseSettingsComponent.hpp"
#include "DiffusionOptions.hpp"
#include "UISchema.hpp"
#include <string>
#include <fstream>
#include <filesystem>

namespace ECS {

    class SDCPPSettingsComponent : public BaseSettingsComponent {
    public:
        bool enable_mmap = true;
        bool flash_attn = true;
        std::string max_vram = "-1";
        bool stream_layers = false;
        bool eager_load = false;
        std::string backend;
        std::string params_backend;
        std::string split_mode;
        bool auto_fit = false;
        std::string rpc_servers;
        std::string lora_apply_mode = "LORA_APPLY_AUTO";
        bool diffusion_flash_attn = false;
        bool diffusion_conv_direct = false;
        bool vae_conv_direct = false;
        bool force_sdxl_vae_conv_scale = false;
        int log_level = 1;
        std::string model_args;

        SDCPPSettingsComponent() {
            compName = "SDCPP";
            compCategory = "SDCPP";

            schema = {
                {"title", "Global SDCPP Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "enable_mmap", "max_vram", "stream_layers", "eager_load",
                    "backend", "params_backend", "split_mode", "auto_fit",
                    "rpc_servers", "lora_apply_mode", "diffusion_flash_attn",
                    "diffusion_conv_direct", "vae_conv_direct", "force_sdxl_vae_conv_scale",
                    "log_level", "model_args"
                }},
                {"properties", {
                    {"enable_mmap", {
                        {"type", "boolean"},
                        {"title", "Enable mmap"},
                        {"description", "Use memory-mapped files for model loading (improves performance for large models)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"max_vram", {
                        {"type", "string"},
                        {"title", "Max VRAM"},
                        {"description", "Maximum VRAM to use (e.g., '4GB', '8GB', '-1' for unlimited)."},
                        {"ui:widget", "text"}
                    }},
                    {"stream_layers", {
                        {"type", "boolean"},
                        {"title", "Stream Layers"},
                        {"description", "Stream layers from disk during inference (reduces VRAM usage)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"eager_load", {
                        {"type", "boolean"},
                        {"title", "Eager Load"},
                        {"description", "Load all models eagerly at startup (faster inference, higher VRAM usage)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"backend", {
                        {"type", "string"},
                        {"title", "Backend"},
                        {"description", "Computation backend (e.g., 'cuda', 'cpu', 'auto')."},
                        {"ui:widget", "text"}
                    }},
                    {"params_backend", {
                        {"type", "string"},
                        {"title", "Params Backend"},
                        {"description", "Backend for parameter storage (e.g., 'cuda', 'cpu')."},
                        {"ui:widget", "text"}
                    }},
                    {"split_mode", {
                        {"type", "string"},
                        {"title", "Split Mode"},
                        {"description", "Model splitting mode for multi-GPU (e.g., 'none', 'layer', 'tensor')."},
                        {"ui:widget", "text"}
                    }},
                    {"auto_fit", {
                        {"type", "boolean"},
                        {"title", "Auto Fit"},
                        {"description", "Automatically fit model to available VRAM when loading."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"rpc_servers", {
                        {"type", "string"},
                        {"title", "RPC Servers"},
                        {"description", "Comma-separated list of RPC server addresses for distributed inference."},
                        {"ui:widget", "text"}
                    }},
                    {"lora_apply_mode", {
                        {"type", "string"},
                        {"title", "LoRA Apply Mode"},
                        {"description", "When to apply LoRA weights (Auto, Immediately, or At Runtime)."},
                        {"ui:widget", "combo"},
                        {"items", get_lora_apply_mode_names()},
                        {"itemCount", static_cast<int>(get_lora_apply_mode_names().size())}
                    }},
                    {"flash_attn", {
                        {"type", "boolean"},
                        {"title", "Flash Attention"},
                        {"description", "Enable Flash Attention for text encoders and VAE (faster, lower memory)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"diffusion_flash_attn", {
                        {"type", "boolean"},
                        {"title", "Diffusion Flash Attention"},
                        {"description", "Enable Flash Attention for diffusion model (faster, lower memory)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"diffusion_conv_direct", {
                        {"type", "boolean"},
                        {"title", "Diffusion Conv Direct"},
                        {"description", "Use direct convolution implementation (may be faster on some hardware)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"vae_conv_direct", {
                        {"type", "boolean"},
                        {"title", "VAE Conv Direct"},
                        {"description", "Use direct convolution for VAE (may improve performance)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"force_sdxl_vae_conv_scale", {
                        {"type", "boolean"},
                        {"title", "Force SDXL VAE Conv Scale"},
                        {"description", "Force SDXL VAE convolution scaling (fixes some compatibility issues)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"log_level", {
                        {"type", "integer"},
                        {"title", "Log Level"},
                        {"description", "Minimum log level to display (0=DEBUG,1=INFO,2=WARN,3=ERROR)."},
                        {"ui:widget", "combo"},
                        {"items", {"DEBUG","INFO","WARN","ERROR"}},
                        {"itemCount", 4}
                    }},
                    {"model_args", {
                        {"type", "string"},
                        {"title", "Model Args"},
                        {"description", "Additional command-line style arguments for the model backend (e.g., --disable-async-offload --disable-pinned-memory)."},
                        {"ui:widget", "text"}
                    }}
                }}
            };
        }

        bool SaveSettings() override {
            nlohmann::json j = Serialize();
            std::string path = GetSettingsDirectory() + "/sdcpp.json";
            std::ofstream file(path);
            if (!file.is_open()) return false;
            file << j.dump(4);
            return true;
        }

        bool LoadSettings() override {
            std::string path = GetSettingsDirectory() + "/sdcpp.json";
            std::ifstream file(path);
            if (!file.is_open()) return false;
            nlohmann::json j;
            file >> j;
            Deserialize(j);
            return true;
        }

        void ResetToDefaults() override {
            *this = SDCPPSettingsComponent();
        }

        void CreateBackup() override {
            backupJson = Serialize();
        }

        void RestoreFromBackup() override {
            if (!backupJson.is_null()) Deserialize(backupJson);
        }

        bool HasUnsavedChanges() const override {
            nlohmann::json current = Serialize();
            return current != backupJson;
        }

        nlohmann::json Serialize() const override {
            return {
                {"enable_mmap", enable_mmap},
                {"max_vram", max_vram},
                {"flash_attn", flash_attn},
                {"stream_layers", stream_layers},
                {"eager_load", eager_load},
                {"backend", backend},
                {"params_backend", params_backend},
                {"split_mode", split_mode},
                {"auto_fit", auto_fit},
                {"rpc_servers", rpc_servers},
                {"lora_apply_mode", lora_apply_mode},
                {"diffusion_flash_attn", diffusion_flash_attn},
                {"diffusion_conv_direct", diffusion_conv_direct},
                {"vae_conv_direct", vae_conv_direct},
                {"force_sdxl_vae_conv_scale", force_sdxl_vae_conv_scale},
                {"log_level", log_level},
                {"model_args", model_args}
            };
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains("enable_mmap")) enable_mmap = j["enable_mmap"].get<bool>();
            if (j.contains("max_vram")) max_vram = j["max_vram"].get<std::string>();
            if (j.contains("flash_attn")) flash_attn = j["flash_attn"].get<bool>();
            if (j.contains("stream_layers")) stream_layers = j["stream_layers"].get<bool>();
            if (j.contains("eager_load")) eager_load = j["eager_load"].get<bool>();
            if (j.contains("backend")) backend = j["backend"].get<std::string>();
            if (j.contains("params_backend")) params_backend = j["params_backend"].get<std::string>();
            if (j.contains("split_mode")) split_mode = j["split_mode"].get<std::string>();
            if (j.contains("auto_fit")) auto_fit = j["auto_fit"].get<bool>();
            if (j.contains("rpc_servers")) rpc_servers = j["rpc_servers"].get<std::string>();

            if (j.contains("lora_apply_mode")) {
                const auto& val = j["lora_apply_mode"];
                if (val.is_string()) lora_apply_mode = val.get<std::string>();
            }

            if (j.contains("diffusion_flash_attn")) diffusion_flash_attn = j["diffusion_flash_attn"].get<bool>();
            if (j.contains("diffusion_conv_direct")) diffusion_conv_direct = j["diffusion_conv_direct"].get<bool>();
            if (j.contains("vae_conv_direct")) vae_conv_direct = j["vae_conv_direct"].get<bool>();
            if (j.contains("force_sdxl_vae_conv_scale")) force_sdxl_vae_conv_scale = j["force_sdxl_vae_conv_scale"].get<bool>();
            if (j.contains("log_level")) log_level = j["log_level"].get<int>();
            if (j.contains("model_args")) model_args = j["model_args"].get<std::string>();
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"enable_mmap", &enable_mmap},
                {"max_vram", &max_vram},
                {"flash_attn", &flash_attn},
                {"stream_layers", &stream_layers},
                {"eager_load", &eager_load},
                {"backend", &backend},
                {"params_backend", &params_backend},
                {"split_mode", &split_mode},
                {"auto_fit", &auto_fit},
                {"rpc_servers", &rpc_servers},
                {"lora_apply_mode", &lora_apply_mode},
                {"diffusion_flash_attn", &diffusion_flash_attn},
                {"diffusion_conv_direct", &diffusion_conv_direct},
                {"vae_conv_direct", &vae_conv_direct},
                {"force_sdxl_vae_conv_scale", &force_sdxl_vae_conv_scale},
                {"log_level", &log_level},
                {"model_args", &model_args}
            };
        }

    private:
        nlohmann::json backupJson;
    };

} // namespace ECS