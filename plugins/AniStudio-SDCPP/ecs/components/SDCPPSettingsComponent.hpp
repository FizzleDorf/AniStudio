#pragma once

#include "BaseSettingsComponent.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>
#include <fstream>
#include <filesystem>

namespace ECS {

    class SDCPPSettingsComponent : public BaseSettingsComponent {
    public:
        enum lora_apply_mode_t lora_apply_mode = LORA_APPLY_AUTO;
        bool enable_mmap = true;
        bool flash_attn = false;
        bool diffusion_flash_attn = false;
        bool tae_preview_only = false;
        bool diffusion_conv_direct = false;
        bool vae_conv_direct = false;
        bool force_sdxl_vae_conv_scale = false;
        enum sd_vae_format_t vae_format = SD_VAE_FORMAT_AUTO;
        std::string max_vram = "-1";
        bool stream_layers = false;
        bool eager_load = false;
        std::string backend;
        std::string params_backend;
        std::string split_mode;
        bool auto_fit = false;
        std::string rpc_servers;

        SDCPPSettingsComponent() {
            compName = "SDCPP";
            compCategory = "SDCPP";
        }

        std::string GetTabName() const override { return "SDCPP"; }
        std::string GetTabCategory() const override { return "SDCPP"; }

        void RenderUI() override {
            ImGui::Text("Global SDCPP settings applied at context creation");
            ImGui::Separator();

            int lora_mode = static_cast<int>(lora_apply_mode);
            if (ImGui::Combo("LoRA Apply Mode", &lora_mode, lora_apply_mode_items, lora_apply_mode_item_count))
                lora_apply_mode = static_cast<lora_apply_mode_t>(lora_mode);

            ImGui::Checkbox("Enable mmap", &enable_mmap);
            ImGui::Checkbox("Flash Attention (general)", &flash_attn);
            ImGui::Checkbox("Diffusion Flash Attention", &diffusion_flash_attn);
            ImGui::Checkbox("TAE Preview Only", &tae_preview_only);
            ImGui::Checkbox("Diffusion Conv Direct", &diffusion_conv_direct);
            ImGui::Checkbox("VAE Conv Direct", &vae_conv_direct);
            ImGui::Checkbox("Force SDXL VAE Conv Scale", &force_sdxl_vae_conv_scale);

            int vae_fmt = static_cast<int>(vae_format);
            if (ImGui::Combo("VAE Format", &vae_fmt, vae_format_items, vae_format_item_count))
                vae_format = static_cast<sd_vae_format_t>(vae_fmt);

            char maxVramBuf[256];
            strncpy(maxVramBuf, max_vram.c_str(), sizeof(maxVramBuf) - 1);
            maxVramBuf[sizeof(maxVramBuf) - 1] = '\0';
            if (ImGui::InputText("Max VRAM", maxVramBuf, sizeof(maxVramBuf)))
                max_vram = maxVramBuf;

            ImGui::Checkbox("Stream Layers", &stream_layers);
            ImGui::Checkbox("Eager Load", &eager_load);

            char backendBuf[128];
            strncpy(backendBuf, backend.c_str(), sizeof(backendBuf) - 1);
            backendBuf[sizeof(backendBuf) - 1] = '\0';
            if (ImGui::InputText("Backend", backendBuf, sizeof(backendBuf)))
                backend = backendBuf;

            char paramsBackendBuf[128];
            strncpy(paramsBackendBuf, params_backend.c_str(), sizeof(paramsBackendBuf) - 1);
            paramsBackendBuf[sizeof(paramsBackendBuf) - 1] = '\0';
            if (ImGui::InputText("Params Backend", paramsBackendBuf, sizeof(paramsBackendBuf)))
                params_backend = paramsBackendBuf;

            char splitModeBuf[128];
            strncpy(splitModeBuf, split_mode.c_str(), sizeof(splitModeBuf) - 1);
            splitModeBuf[sizeof(splitModeBuf) - 1] = '\0';
            if (ImGui::InputText("Split Mode", splitModeBuf, sizeof(splitModeBuf)))
                split_mode = splitModeBuf;

            ImGui::Checkbox("Auto Fit", &auto_fit);

            char rpcServersBuf[256];
            strncpy(rpcServersBuf, rpc_servers.c_str(), sizeof(rpcServersBuf) - 1);
            rpcServersBuf[sizeof(rpcServersBuf) - 1] = '\0';
            if (ImGui::InputText("RPC Servers", rpcServersBuf, sizeof(rpcServersBuf)))
                rpc_servers = rpcServersBuf;
        }

        void RenderFilteredUI(const std::string& filter) override {
            RenderUI();
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
                {"lora_apply_mode", static_cast<int>(lora_apply_mode)},
                {"enable_mmap", enable_mmap},
                {"flash_attn", flash_attn},
                {"diffusion_flash_attn", diffusion_flash_attn},
                {"tae_preview_only", tae_preview_only},
                {"diffusion_conv_direct", diffusion_conv_direct},
                {"vae_conv_direct", vae_conv_direct},
                {"force_sdxl_vae_conv_scale", force_sdxl_vae_conv_scale},
                {"vae_format", static_cast<int>(vae_format)},
                {"max_vram", max_vram},
                {"stream_layers", stream_layers},
                {"eager_load", eager_load},
                {"backend", backend},
                {"params_backend", params_backend},
                {"split_mode", split_mode},
                {"auto_fit", auto_fit},
                {"rpc_servers", rpc_servers}
            };
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains("lora_apply_mode"))
                lora_apply_mode = static_cast<lora_apply_mode_t>(j["lora_apply_mode"].get<int>());
            if (j.contains("enable_mmap")) enable_mmap = j["enable_mmap"].get<bool>();
            if (j.contains("flash_attn")) flash_attn = j["flash_attn"].get<bool>();
            if (j.contains("diffusion_flash_attn")) diffusion_flash_attn = j["diffusion_flash_attn"].get<bool>();
            if (j.contains("tae_preview_only")) tae_preview_only = j["tae_preview_only"].get<bool>();
            if (j.contains("diffusion_conv_direct")) diffusion_conv_direct = j["diffusion_conv_direct"].get<bool>();
            if (j.contains("vae_conv_direct")) vae_conv_direct = j["vae_conv_direct"].get<bool>();
            if (j.contains("force_sdxl_vae_conv_scale")) force_sdxl_vae_conv_scale = j["force_sdxl_vae_conv_scale"].get<bool>();
            if (j.contains("vae_format"))
                vae_format = static_cast<sd_vae_format_t>(j["vae_format"].get<int>());
            if (j.contains("max_vram")) max_vram = j["max_vram"].get<std::string>();
            if (j.contains("stream_layers")) stream_layers = j["stream_layers"].get<bool>();
            if (j.contains("eager_load")) eager_load = j["eager_load"].get<bool>();
            if (j.contains("backend")) backend = j["backend"].get<std::string>();
            if (j.contains("params_backend")) params_backend = j["params_backend"].get<std::string>();
            if (j.contains("split_mode")) split_mode = j["split_mode"].get<std::string>();
            if (j.contains("auto_fit")) auto_fit = j["auto_fit"].get<bool>();
            if (j.contains("rpc_servers")) rpc_servers = j["rpc_servers"].get<std::string>();
        }

    private:
        nlohmann::json backupJson;
    };

} // namespace ECS